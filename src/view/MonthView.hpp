#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <optional>
#include <vector>

#include "core/MonthSheet.hpp"

namespace ht::view {

/// Draws one MonthSheet and turns clicks back into (habit, day) pairs.
///
/// It owns no habit data: every method takes the sheet it should reflect, so the
/// model stays the single source of truth. All squares are drawn from one
/// VertexArray -- a single draw call for the whole grid, rather than one per
/// square -- and hit-testing is arithmetic rather than a scan.
class MonthView final {
public:
    struct Hit final {
        std::size_t habitIndex = 0;
        unsigned day = 0;  ///< Zero-based.
    };

    MonthView(const sf::Font& font, sf::Vector2f origin);

    MonthView() = delete;

    /// Rebuilds everything: names, day header, and squares. Call when the sheet's
    /// month, habit list, or day count changes.
    /// `today` is the zero-based day to highlight, if this sheet is the current
    /// month.
    void setSheet(const MonthSheet& sheet, std::optional<unsigned> today);

    /// Recolours the squares in place. Call after a mark changes.
    /// Precondition: `sheet` has the same shape as the one last passed to
    /// setSheet(); throws std::invalid_argument if it does not.
    void refreshMarks(const MonthSheet& sheet);

    void draw(sf::RenderTarget& target) const;

    /// The square under `point`, or nullopt if the point is in a gap, in the
    /// name column, or outside the grid.
    [[nodiscard]] std::optional<Hit> hitTest(sf::Vector2f point) const;

    /// Size needed to lay out a grid of this shape, excluding the navigation bar.
    [[nodiscard]] static sf::Vector2f contentSize(std::size_t habitCount,
                                                  unsigned dayCount);

    /// Y coordinate just below the grid.
    [[nodiscard]] float bottom() const;

private:
    [[nodiscard]] sf::Vector2f squarePosition(std::size_t row, unsigned day) const;
    [[nodiscard]] std::size_t vertexBase(std::size_t row, unsigned day) const;
    void rebuildNames(const MonthSheet& sheet);
    void rebuildDayHeader(unsigned dayCount, std::optional<unsigned> today);
    void rebuildSquares(const MonthSheet& sheet, std::optional<unsigned> today);

    const sf::Font* font_;  ///< Borrowed; must outlive this view.
    sf::Vector2f origin_;
    sf::VertexArray squares_{sf::Quads};
    std::vector<sf::Text> habitLabels_;
    std::vector<sf::Text> dayLabels_;
    std::size_t habitCount_ = 0;
    unsigned dayCount_ = 0;
};

}  // namespace ht::view
