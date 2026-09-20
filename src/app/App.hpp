#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "core/Date.hpp"
#include "core/HabitName.hpp"
#include "core/HabitStore.hpp"
#include "view/MonthView.hpp"
#include "view/NavBar.hpp"
#include "view/PromptBar.hpp"

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

    /// The most habit rows the window can show. The window is sized to its
    /// content but capped at theme::kMaxWindowHeight, and nothing scales the
    /// grid to fit, so rows beyond this are drawn off-screen where they cannot
    /// be seen or clicked.
    [[nodiscard]] static std::size_t maxVisibleHabits();

private:
    /// What the next keystroke or click means.
    ///
    /// Only Marking reaches the grid; while a habit is being named or a removal
    /// confirmed, the navigation bar is replaced by the prompt and clicks on the
    /// grid do nothing, so no mark can be toggled by a stray click meant for the
    /// question on screen.
    enum class Mode : std::uint8_t { Marking, NamingHabit, ConfirmingRemoval };

    /// How long changes may sit unsaved. Small enough that a crash costs almost
    /// nothing, large enough that dragging across a row is one write.
    static constexpr float kAutosaveSeconds = 2.0F;

    [[nodiscard]] static sf::Font loadFont(const std::filesystem::path& path);
    [[nodiscard]] static sf::VideoMode videoModeFor(const Year& year);
    [[nodiscard]] static sf::Vector2f navOrigin(const Year& year);
    [[nodiscard]] static float navWidth();

    void handleEvent(const sf::Event& event);
    void onMousePressed(sf::Vector2f point, sf::Mouse::Button button);
    void onTextEntered(std::uint32_t unicode);

    void beginAddHabit();
    void beginRemoveHabit(std::size_t row);
    void commitEdit();
    void commitNewHabit();
    void commitRemoval();
    void cancelEdit();
    void refreshPrompt();
    /// True when another row would not fit in the window.
    [[nodiscard]] bool atRowCapacity() const;
    /// Resizes the window and moves the bars to match the current habit count,
    /// then saves. Called after every change to the habit list.
    void afterHabitListChanged();
    void relayout();
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
    view::PromptBar prompt_;
    sf::Clock sinceSave_;
    Month current_;
    Mode mode_ = Mode::Marking;
    /// The name being typed. Not a HabitName: it is invalid for most of its life.
    std::string typedName_;
    /// The habit a confirmed removal would delete, held by name rather than by
    /// row so it cannot come to mean a different habit.
    std::optional<HabitName> pendingRemoval_;
    std::string notice_;
    /// Set when the naming prompt is opened from the keyboard: the key that
    /// opened it also produces a TextEntered event, which would otherwise type
    /// itself into the field the keystroke just created.
    bool swallowNextText_ = false;
    bool saveFailed_ = false;
};

}  // namespace ht
