#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

namespace ht::view {

/// A labelled clickable box.
///
/// One constructor, which leaves the object fully usable -- there is no
/// default-constructed state in which draw() would render an unfonted label.
/// The font is borrowed, not owned, and must outlive the button.
class Button final {
public:
    Button(const sf::Font& font, const std::string& label, sf::Vector2f position,
           sf::Vector2f size);

    Button() = delete;

    void draw(sf::RenderTarget& target) const;

    [[nodiscard]] bool contains(sf::Vector2f point) const;

    /// Lets the button light up under the cursor. Returns true if the state
    /// changed, so callers can skip needless redraws.
    bool setHovered(bool hovered);
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }

    [[nodiscard]] sf::FloatRect bounds() const { return box_.getGlobalBounds(); }

private:
    sf::RectangleShape box_;
    sf::Text label_;
    bool hovered_ = false;
};

}  // namespace ht::view
