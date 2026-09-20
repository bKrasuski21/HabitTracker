#include "view/Button.hpp"

#include "view/Theme.hpp"

namespace ht::view {

Button::Button(const sf::Font& font, const std::string& label, sf::Vector2f position,
               sf::Vector2f size)
    : box_(size), label_(label, font, theme::kButtonCharacterSize) {
    box_.setPosition(position);
    box_.setFillColor(theme::kButtonFill);
    box_.setOutlineThickness(theme::kOutlineThickness);
    box_.setOutlineColor(theme::kSquareOutline);

    label_.setFillColor(theme::kButtonText);
    // Centre on the glyphs actually drawn, which is why local bounds are offset
    // by their own left/top rather than assumed to start at the origin.
    const sf::FloatRect text = label_.getLocalBounds();
    label_.setPosition(position.x + (size.x - text.width) / 2.F - text.left,
                       position.y + (size.y - text.height) / 2.F - text.top);
}

void Button::draw(sf::RenderTarget& target) const {
    target.draw(box_);
    target.draw(label_);
}

bool Button::contains(sf::Vector2f point) const {
    return box_.getGlobalBounds().contains(point);
}

bool Button::setHovered(bool hovered) {
    if (hovered_ == hovered) {
        return false;
    }
    hovered_ = hovered;
    box_.setFillColor(hovered_ ? theme::kButtonHoverFill : theme::kButtonFill);
    return true;
}

}  // namespace ht::view
