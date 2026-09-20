#include "view/NavBar.hpp"

#include <string>

#include "view/Theme.hpp"

namespace ht::view {

NavBar::NavBar(const sf::Font& font, sf::Vector2f origin, float width)
    : origin_(origin),
      width_(width),
      previous_(font, "<", origin, {theme::kButtonWidth, theme::kButtonHeight}),
      addHabit_(font, "+",
                {origin.x + theme::kButtonWidth + theme::kButtonGap, origin.y},
                {theme::kButtonWidth, theme::kButtonHeight}),
      next_(font, ">", {origin.x + width - theme::kButtonWidth, origin.y},
            {theme::kButtonWidth, theme::kButtonHeight}),
      label_("", font, theme::kLabelCharacterSize) {
    label_.setFillColor(theme::kText);
}

void NavBar::setMonth(Month month, int year) {
    label_.setString(std::string(monthName(month)) + " " + std::to_string(year));
    centreLabel();
}

void NavBar::setOrigin(sf::Vector2f origin) {
    // Buttons position themselves once, at construction, so the whole bar is
    // rebuilt rather than nudged -- there is no half-moved state to get wrong.
    const sf::Font& font = *label_.getFont();
    const sf::String label = label_.getString();
    *this = NavBar(font, origin, width_);
    label_.setString(label);
    centreLabel();
}

void NavBar::centreLabel() {
    // Horizontally on the glyphs, vertically on the font's line box, so the
    // label does not shift between months of different glyph heights.
    const sf::FloatRect bounds = label_.getLocalBounds();
    const float lineHeight = label_.getFont()->getLineSpacing(theme::kLabelCharacterSize);
    label_.setPosition(origin_.x + (width_ - bounds.width) / 2.F - bounds.left,
                       origin_.y + (theme::kButtonHeight - lineHeight) / 2.F);
}

NavBar::Action NavBar::hitTest(sf::Vector2f point) const {
    if (previous_.contains(point)) {
        return Action::Previous;
    }
    if (addHabit_.contains(point)) {
        return Action::AddHabit;
    }
    if (next_.contains(point)) {
        return Action::Next;
    }
    return Action::None;
}

bool NavBar::updateHover(sf::Vector2f point) {
    const bool previousChanged = previous_.setHovered(previous_.contains(point));
    const bool addChanged = addHabit_.setHovered(addHabit_.contains(point));
    const bool nextChanged = next_.setHovered(next_.contains(point));
    return previousChanged || addChanged || nextChanged;
}

void NavBar::draw(sf::RenderTarget& target) const {
    previous_.draw(target);
    addHabit_.draw(target);
    next_.draw(target);
    target.draw(label_);
}

}  // namespace ht::view
