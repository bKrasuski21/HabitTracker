#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace ht::view {

/// The one-line bar that asks a question or takes a habit name.
///
/// It occupies the navigation bar's slot and is drawn in its place while an
/// edit is in progress, so the window does not change height just because the
/// user started typing. Like every other view it owns no habit data: the caller
/// passes the line to show.
class PromptBar final {
public:
    PromptBar(const sf::Font& font, sf::Vector2f origin, float width);

    PromptBar() = delete;

    /// `line` is the whole line to show. `caret` is the character index to draw
    /// a caret at, or nullopt for a question with nothing being typed.
    /// `notice` is appended in the warning colour, for "that name is taken".
    void set(const std::string& line, std::optional<std::size_t> caret,
             const std::string& notice);

    /// Moves the bar, for when adding or removing a habit resizes the window.
    void setOrigin(sf::Vector2f origin);

    void draw(sf::RenderTarget& target) const;

private:
    void layout();

    sf::Vector2f origin_;
    float width_;
    sf::RectangleShape box_;
    sf::Text line_;
    sf::Text notice_;
    sf::RectangleShape caret_;
    std::optional<std::size_t> caretIndex_;
};

}  // namespace ht::view
