#include "view/PromptBar.hpp"

#include "view/Theme.hpp"

namespace ht::view {
namespace {

/// Padding between the bar's edge and its text.
constexpr float kPadding = 12.F;

}  // namespace

PromptBar::PromptBar(const sf::Font& font, sf::Vector2f origin, float width)
    : origin_(origin),
      width_(width),
      box_({width, theme::kNavBarHeight}),
      line_("", font, theme::kPromptCharacterSize),
      notice_("", font, theme::kPromptCharacterSize),
      caret_({theme::kOutlineThickness, static_cast<float>(theme::kPromptCharacterSize)}) {
    box_.setFillColor(theme::kPromptFill);
    box_.setOutlineThickness(theme::kOutlineThickness);
    box_.setOutlineColor(theme::kSquareOutline);
    line_.setFillColor(theme::kText);
    notice_.setFillColor(theme::kNoticeText);
    caret_.setFillColor(theme::kCaret);
    layout();
}

void PromptBar::set(const std::string& line, std::optional<std::size_t> caret,
                    const std::string& notice) {
    line_.setString(line);
    notice_.setString(notice);
    caretIndex_ = caret;
    layout();
}

void PromptBar::setOrigin(sf::Vector2f origin) {
    origin_ = origin;
    layout();
}

void PromptBar::layout() {
    box_.setPosition(origin_);
    box_.setSize({width_, theme::kNavBarHeight});

    // Vertically centred on the font's line box rather than on the glyphs, so
    // the text does not hop as characters are typed.
    const float lineHeight = line_.getFont()->getLineSpacing(theme::kPromptCharacterSize);
    const float baseline = origin_.y + (theme::kNavBarHeight - lineHeight) / 2.F;
    line_.setPosition(origin_.x + kPadding, baseline);

    if (caretIndex_.has_value()) {
        // findCharacterPos() accounts for kerning, so the caret sits where the
        // next glyph will actually be drawn.
        const sf::Vector2f pen = line_.findCharacterPos(*caretIndex_);
        caret_.setPosition(pen.x, baseline + 2.F);
    }

    // The notice trails the line, far enough not to be mistaken for typed text.
    const sf::FloatRect lineBounds = line_.getLocalBounds();
    notice_.setPosition(origin_.x + kPadding + lineBounds.left + lineBounds.width + 24.F,
                        baseline);
}

void PromptBar::draw(sf::RenderTarget& target) const {
    target.draw(box_);
    target.draw(line_);
    if (caretIndex_.has_value()) {
        target.draw(caret_);
    }
    if (!notice_.getString().isEmpty()) {
        target.draw(notice_);
    }
}

}  // namespace ht::view
