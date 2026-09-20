#include "view/MonthView.hpp"

#include <stdexcept>
#include <string>

#include "view/Theme.hpp"

namespace ht::view {
namespace {

/// Vertices per square: four for the outline quad, four for the fill drawn
/// inside it.
constexpr std::size_t kVerticesPerSquare = 8;

void setQuad(sf::VertexArray& vertices, std::size_t base, sf::Vector2f topLeft,
             float size, const sf::Color& color) {
    vertices[base + 0].position = topLeft;
    vertices[base + 1].position = {topLeft.x + size, topLeft.y};
    vertices[base + 2].position = {topLeft.x + size, topLeft.y + size};
    vertices[base + 3].position = {topLeft.x, topLeft.y + size};
    for (std::size_t i = 0; i < 4; ++i) {
        vertices[base + i].color = color;
    }
}

void setQuadColor(sf::VertexArray& vertices, std::size_t base, const sf::Color& color) {
    for (std::size_t i = 0; i < 4; ++i) {
        vertices[base + i].color = color;
    }
}

}  // namespace

MonthView::MonthView(const sf::Font& font, sf::Vector2f origin)
    : font_(&font), origin_(origin) {}

sf::Vector2f MonthView::contentSize(std::size_t habitCount, unsigned dayCount) {
    const float width =
        theme::kNameColumnWidth +
        (dayCount == 0 ? 0.F
                       : static_cast<float>(dayCount - 1) * theme::kSquarePitch +
                             theme::kSquareSize);
    const float height =
        theme::kDayHeaderHeight +
        (habitCount == 0 ? 0.F
                         : static_cast<float>(habitCount - 1) * theme::kRowPitch +
                               theme::kSquareSize);
    return {width, height};
}

float MonthView::bottom() const {
    return origin_.y + contentSize(habitCount_, dayCount_).y;
}

sf::Vector2f MonthView::squarePosition(std::size_t row, unsigned day) const {
    return {origin_.x + theme::kNameColumnWidth +
                static_cast<float>(day) * theme::kSquarePitch,
            origin_.y + theme::kDayHeaderHeight +
                static_cast<float>(row) * theme::kRowPitch};
}

std::size_t MonthView::vertexBase(std::size_t row, unsigned day) const {
    return (row * dayCount_ + day) * kVerticesPerSquare;
}

void MonthView::setSheet(const MonthSheet& sheet, std::optional<unsigned> today) {
    habitCount_ = sheet.habitCount();
    dayCount_ = sheet.dayCount();
    rebuildNames(sheet);
    rebuildDayHeader(dayCount_, today);
    rebuildSquares(sheet, today);
}

void MonthView::rebuildNames(const MonthSheet& sheet) {
    habitLabels_.clear();
    habitLabels_.reserve(sheet.habitCount());
    for (std::size_t row = 0; row < sheet.habitCount(); ++row) {
        sf::Text label(sheet.track(row).name().str(), *font_, theme::kHabitCharacterSize);
        label.setFillColor(theme::kText);
        // Centred on the font's line box rather than on this string's glyph
        // bounds, so every row shares a baseline. Centring on per-string bounds
        // makes a name with a descender sit higher than one without.
        const float lineHeight = font_->getLineSpacing(theme::kHabitCharacterSize);
        label.setPosition(origin_.x,
                          squarePosition(row, 0).y +
                              (theme::kSquareSize - lineHeight) / 2.F);
        habitLabels_.push_back(std::move(label));
    }
}

void MonthView::rebuildDayHeader(unsigned dayCount, std::optional<unsigned> today) {
    dayLabels_.clear();
    dayLabels_.reserve(dayCount);
    for (unsigned day = 0; day < dayCount; ++day) {
        sf::Text label(std::to_string(day + 1), *font_, theme::kDayCharacterSize);
        label.setFillColor(today.has_value() && *today == day ? theme::kTodayOutline
                                                              : theme::kMutedText);
        // Horizontally centred on the glyphs so "1" and "31" both sit over their
        // column; vertically fixed, since every label is digits of one size.
        const sf::FloatRect bounds = label.getLocalBounds();
        const sf::Vector2f square = squarePosition(0, day);
        label.setPosition(square.x + (theme::kSquareSize - bounds.width) / 2.F - bounds.left,
                          origin_.y);
        dayLabels_.push_back(std::move(label));
    }
}

void MonthView::rebuildSquares(const MonthSheet& sheet, std::optional<unsigned> today) {
    squares_.clear();
    squares_.resize(habitCount_ * dayCount_ * kVerticesPerSquare);

    const float inset = theme::kOutlineThickness;
    for (std::size_t row = 0; row < habitCount_; ++row) {
        const HabitTrack& track = sheet.track(row);
        for (unsigned day = 0; day < dayCount_; ++day) {
            const sf::Vector2f position = squarePosition(row, day);
            const std::size_t base = vertexBase(row, day);
            const bool isToday = today.has_value() && *today == day;
            setQuad(squares_, base, position, theme::kSquareSize,
                    isToday ? theme::kTodayOutline : theme::kSquareOutline);
            setQuad(squares_, base + 4, {position.x + inset, position.y + inset},
                    theme::kSquareSize - 2.F * inset, theme::markColor(track.mark(day)));
        }
    }
}

void MonthView::refreshMarks(const MonthSheet& sheet) {
    if (sheet.habitCount() != habitCount_ || sheet.dayCount() != dayCount_) {
        throw std::invalid_argument(
            "refreshMarks called with a sheet of a different shape; call setSheet");
    }
    for (std::size_t row = 0; row < habitCount_; ++row) {
        const HabitTrack& track = sheet.track(row);
        for (unsigned day = 0; day < dayCount_; ++day) {
            setQuadColor(squares_, vertexBase(row, day) + 4,
                         theme::markColor(track.mark(day)));
        }
    }
}

void MonthView::draw(sf::RenderTarget& target) const {
    target.draw(squares_);
    for (const sf::Text& label : dayLabels_) {
        target.draw(label);
    }
    for (const sf::Text& label : habitLabels_) {
        target.draw(label);
    }
}

std::optional<MonthView::Hit> MonthView::hitTest(sf::Vector2f point) const {
    if (habitCount_ == 0 || dayCount_ == 0) {
        return std::nullopt;
    }

    const sf::Vector2f gridOrigin = squarePosition(0, 0);
    const float dx = point.x - gridOrigin.x;
    const float dy = point.y - gridOrigin.y;
    if (dx < 0.F || dy < 0.F) {
        return std::nullopt;
    }

    // Which cell of the pitch grid, then whether the point is on the square
    // rather than in the gap between squares.
    const auto column = static_cast<unsigned>(dx / theme::kSquarePitch);
    const auto row = static_cast<std::size_t>(dy / theme::kRowPitch);
    if (column >= dayCount_ || row >= habitCount_) {
        return std::nullopt;
    }
    if (dx - static_cast<float>(column) * theme::kSquarePitch > theme::kSquareSize ||
        dy - static_cast<float>(row) * theme::kRowPitch > theme::kSquareSize) {
        return std::nullopt;
    }
    return Hit{row, column};
}

}  // namespace ht::view
