#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <filesystem>
#include <optional>

#include "core/Date.hpp"
#include "core/HabitStore.hpp"
#include "view/MonthView.hpp"
#include "view/NavBar.hpp"

namespace ht {

/// The window, the event loop, and nothing else.
///
/// It borrows the store rather than owning it, so the store's RAII save happens
/// after the window is gone; and it holds no habit data of its own, so the model
/// stays the single source of truth.
class App final {
public:
    struct Config final {
        std::filesystem::path dataDirectory;
        std::filesystem::path fontPath;
        int year = kMinYear;
        Month startMonth = Month::January;
    };

    /// Throws std::runtime_error if the font cannot be loaded.
    /// `store` must outlive the App.
    App(const Config& config, HabitStore& store);

    App() = delete;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    /// Runs until the window closes. Returns EXIT_SUCCESS, or EXIT_FAILURE if
    /// the final save failed.
    [[nodiscard]] int run();

private:
    /// How long changes may sit unsaved. Small enough that a crash costs almost
    /// nothing, large enough that dragging across a row is one write.
    static constexpr float kAutosaveSeconds = 2.0F;

    [[nodiscard]] static sf::Font loadFont(const std::filesystem::path& path);
    [[nodiscard]] static sf::VideoMode videoModeFor(const Year& year);
    [[nodiscard]] static sf::Vector2f navOrigin(const Year& year);
    [[nodiscard]] static float navWidth();

    void handleEvent(const sf::Event& event);
    void onMousePressed(sf::Vector2f point, sf::Mouse::Button button);
    void showMonth(Month month);
    void stepMonth(int step);
    /// The day to highlight when `month` is the real current month.
    [[nodiscard]] std::optional<unsigned> todayIn(Month month) const;
    void render();

    HabitStore& store_;
    sf::Font font_;
    sf::RenderWindow window_;
    view::MonthView grid_;
    view::NavBar nav_;
    sf::Clock sinceSave_;
    Month current_;
    bool saveFailed_ = false;
};

}  // namespace ht
