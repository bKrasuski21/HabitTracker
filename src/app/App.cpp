#include "app/App.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "view/Theme.hpp"

namespace ht {
namespace {

/// Every month is laid out for the longest possible month, so the window does
/// not change size as the user steps through the year.
constexpr unsigned kLayoutDays = kMaxDaysInMonth;

[[nodiscard]] sf::Vector2f contentSizeFor(const Year& year) {
    return view::MonthView::contentSize(std::max<std::size_t>(year.habitCount(), 1),
                                        kLayoutDays);
}

}  // namespace

sf::Font App::loadFont(const std::filesystem::path& path) {
    sf::Font font;
    if (!font.loadFromFile(path.string())) {
        throw std::runtime_error("could not load the font at " + path.string());
    }
    return font;
}

float App::navWidth() {
    return view::MonthView::contentSize(1, kLayoutDays).x;
}

sf::Vector2f App::navOrigin(const Year& year) {
    return {theme::kMargin,
            theme::kMargin + contentSizeFor(year).y + theme::kNavBarGap};
}

sf::VideoMode App::videoModeFor(const Year& year) {
    const sf::Vector2f content = contentSizeFor(year);
    const float width = 2.F * theme::kMargin + content.x;
    const float height = 2.F * theme::kMargin + content.y + theme::kNavBarGap +
                         theme::kNavBarHeight;
    return sf::VideoMode(
        std::min(static_cast<unsigned>(width), theme::kMaxWindowWidth),
        std::min(static_cast<unsigned>(height), theme::kMaxWindowHeight));
}

App::App(const Config& config, HabitStore& store)
    : store_(store),
      font_(loadFont(config.fontPath)),
      window_(videoModeFor(store.year()),
              "HabitTracker " + std::to_string(store.year().number()),
              sf::Style::Titlebar | sf::Style::Close),
      grid_(font_, {theme::kMargin, theme::kMargin}),
      nav_(font_, navOrigin(store.year()), navWidth()),
      current_(config.startMonth) {
    window_.setFramerateLimit(60);
    showMonth(current_);
}

std::optional<unsigned> App::todayIn(Month month) const {
    const YearMonthDay today = currentDate();
    if (today.year != store_.year().number() || today.month != month) {
        return std::nullopt;
    }
    return today.day;
}

void App::showMonth(Month month) {
    current_ = month;
    grid_.setSheet(store_.year().month(month), todayIn(month));
    nav_.setMonth(month, store_.year().number());
}

void App::stepMonth(int step) { showMonth(monthStep(current_, step)); }

void App::onMousePressed(sf::Vector2f point, sf::Mouse::Button button) {
    switch (nav_.hitTest(point)) {
        case view::NavBar::Action::Previous: stepMonth(-1); return;
        case view::NavBar::Action::Next: stepMonth(1); return;
        case view::NavBar::Action::None: break;
    }

    // Only the two buttons that mean something reach the model; the rest are
    // ignored rather than folded into "not left".
    Mark requested = Mark::None;
    if (button == sf::Mouse::Left) {
        requested = Mark::Done;
    } else if (button == sf::Mouse::Right) {
        requested = Mark::Partial;
    } else {
        return;
    }

    const std::optional<view::MonthView::Hit> hit = grid_.hitTest(point);
    if (!hit.has_value()) {
        return;
    }
    store_.mutableYear().toggle(current_, hit->habitIndex, hit->day, requested);
    grid_.refreshMarks(store_.year().month(current_));
}

void App::handleEvent(const sf::Event& event) {
    switch (event.type) {
        case sf::Event::Closed:
            window_.close();
            break;

        case sf::Event::MouseButtonPressed:
            onMousePressed(window_.mapPixelToCoords(
                               {event.mouseButton.x, event.mouseButton.y}),
                           event.mouseButton.button);
            break;

        case sf::Event::MouseMoved:
            nav_.updateHover(
                window_.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}));
            break;

        case sf::Event::KeyPressed:
            switch (event.key.code) {
                case sf::Keyboard::Left: stepMonth(-1); break;
                case sf::Keyboard::Right: stepMonth(1); break;
                case sf::Keyboard::Escape: window_.close(); break;
                case sf::Keyboard::S:
                    if (event.key.control) {
                        saveFailed_ = !store_.saveIfDirty();
                    }
                    break;
                default: break;
            }
            break;

        default:
            break;
    }
}

void App::render() {
    window_.clear(theme::kBackground);
    grid_.draw(window_);
    nav_.draw(window_);
    window_.display();
}

int App::run() {
    while (window_.isOpen()) {
        sf::Event event{};
        while (window_.pollEvent(event)) {
            handleEvent(event);
        }

        // Changes are flushed shortly after they happen, so a crash or a kill
        // costs at most a couple of seconds rather than the whole session.
        if (store_.dirty() && sinceSave_.getElapsedTime().asSeconds() >= kAutosaveSeconds) {
            saveFailed_ = !store_.save();
            sinceSave_.restart();
        }

        render();
    }

    if (!store_.saveIfDirty()) {
        std::cerr << "HabitTracker: could not save to " << store_.filePath().string()
                  << ": " << store_.lastError() << '\n';
        return EXIT_FAILURE;
    }
    return saveFailed_ ? EXIT_FAILURE : EXIT_SUCCESS;
}

}  // namespace ht
