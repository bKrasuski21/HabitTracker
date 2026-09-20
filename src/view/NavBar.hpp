#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/Date.hpp"
#include "view/Button.hpp"

namespace ht::view {

/// The month stepper below the grid: previous, the month and year, next.
class NavBar final {
public:
    enum class Action { None, Previous, Next, AddHabit };

    NavBar(const sf::Font& font, sf::Vector2f origin, float width);

    NavBar() = delete;

    void setMonth(Month month, int year);

    /// Moves the bar, for when adding or removing a habit resizes the window.
    void setOrigin(sf::Vector2f origin);

    /// Which control `point` is over, if any.
    [[nodiscard]] Action hitTest(sf::Vector2f point) const;

    /// Updates hover highlighting. Returns true if anything changed.
    bool updateHover(sf::Vector2f point);

    void draw(sf::RenderTarget& target) const;

private:
    void centreLabel();

    sf::Vector2f origin_;
    float width_;
    Button previous_;
    Button addHabit_;
    Button next_;
    sf::Text label_;
};

}  // namespace ht::view
