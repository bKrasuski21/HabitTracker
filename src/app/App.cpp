#include "app/App.hpp"

#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/HabitEdits.hpp"
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

std::size_t App::maxVisibleHabits() {
    // Everything videoModeFor() adds around the rows themselves; the first row
    // costs kSquareSize and each one after it costs kRowPitch.
    const float chrome = 2.F * theme::kMargin + theme::kDayHeaderHeight +
                         theme::kSquareSize + theme::kNavBarGap + theme::kNavBarHeight;
    const float forRows = static_cast<float>(theme::kMaxWindowHeight) - chrome;
    if (forRows < 0.F) {
        return 1;
    }
    return 1 + static_cast<std::size_t>(forRows / theme::kRowPitch);
}

App::App(const Config& config, HabitStore& store)
    : store_(store),
      font_(loadFont(config.fontPath)),
      window_(videoModeFor(store.year()),
              "HabitTracker " + std::to_string(store.year().number()),
              sf::Style::Titlebar | sf::Style::Close),
      grid_(font_, {theme::kMargin, theme::kMargin}),
      nav_(font_, navOrigin(store.year()), navWidth()),
      prompt_(font_, navOrigin(store.year()), navWidth()),
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

bool App::atRowCapacity() const {
    return store_.year().habitCount() >= maxVisibleHabits();
}

void App::relayout() {
    // The window was sized to the habit count at construction; a habit added or
    // removed changes that, so the window, its view, and the bars below the grid
    // all move together.
    const sf::VideoMode mode = videoModeFor(store_.year());
    window_.setSize({mode.width, mode.height});
    window_.setView(sf::View(sf::FloatRect(0.F, 0.F, static_cast<float>(mode.width),
                                           static_cast<float>(mode.height))));

    const sf::Vector2f origin = navOrigin(store_.year());
    nav_.setOrigin(origin);
    prompt_.setOrigin(origin);
    showMonth(current_);
}

void App::afterHabitListChanged() {
    relayout();
    // Saved at once rather than left to the autosave: a changed habit list is
    // structural, and losing it to a crash would cost more than a stray mark.
    saveFailed_ = !store_.save();
    sinceSave_.restart();
}

void App::beginAddHabit() {
    typedName_.clear();
    // Said up front rather than after the name is typed, so nobody composes a
    // habit only to be told there is no room for it.
    notice_ = atRowCapacity() ? "no room for another row -- remove one first" : "";
    mode_ = Mode::NamingHabit;
    refreshPrompt();
}

void App::beginRemoveHabit(std::size_t row) {
    const MonthSheet& sheet = store_.year().month(current_);
    if (row >= sheet.habitCount()) {
        return;
    }
    pendingRemoval_ = sheet.track(row).name();
    notice_.clear();
    mode_ = Mode::ConfirmingRemoval;
    refreshPrompt();
}

void App::cancelEdit() {
    mode_ = Mode::Marking;
    typedName_.clear();
    pendingRemoval_.reset();
    notice_.clear();
    swallowNextText_ = false;
}

void App::commitEdit() {
    switch (mode_) {
        case Mode::NamingHabit: commitNewHabit(); break;
        case Mode::ConfirmingRemoval: commitRemoval(); break;
        case Mode::Marking: break;
    }
}

void App::commitNewHabit() {
    const std::optional<HabitName> name = HabitName::tryMake(typedName_);
    if (!name.has_value()) {
        notice_ = HabitName::rejectionReason(typedName_);
        refreshPrompt();
        return;
    }
    if (atRowCapacity()) {
        notice_ = "no room for another row -- remove one first";
        refreshPrompt();
        return;
    }

    // Validated through the same core path the command line uses, so "already
    // tracked" reads the same in both places.
    const std::vector<HabitEdit> edits{HabitEdit{HabitEdit::Kind::Add, *name}};
    const HabitEditReport plan = planHabitEdits(store_.year(), edits);
    if (!plan.ok()) {
        notice_ = plan.problems.front();
        refreshPrompt();
        return;
    }

    (void)applyHabitEdits(store_.mutableYear(), edits);
    cancelEdit();
    afterHabitListChanged();
}

void App::commitRemoval() {
    if (!pendingRemoval_.has_value()) {
        cancelEdit();
        return;
    }
    const std::vector<HabitEdit> edits{
        HabitEdit{HabitEdit::Kind::Remove, *pendingRemoval_}};
    (void)applyHabitEdits(store_.mutableYear(), edits);
    cancelEdit();
    afterHabitListChanged();
}

void App::refreshPrompt() {
    switch (mode_) {
        case Mode::NamingHabit: {
            const std::string line = "New habit: " + typedName_;
            prompt_.set(line, line.size(),
                        notice_.empty() ? "Enter to add, Esc to cancel" : notice_);
            break;
        }
        case Mode::ConfirmingRemoval: {
            if (!pendingRemoval_.has_value()) {
                break;
            }
            const std::size_t marks = markedDayCount(store_.year(), *pendingRemoval_);
            prompt_.set("Remove \"" + pendingRemoval_->str() + "\" and its " +
                            std::to_string(marks) +
                            (marks == 1 ? " marked day?" : " marked days?"),
                        std::nullopt, "Enter to remove, Esc to keep");
            break;
        }
        case Mode::Marking:
            break;
    }
}

void App::onTextEntered(std::uint32_t unicode) {
    if (swallowNextText_) {
        swallowNextText_ = false;
        return;
    }

    constexpr std::uint32_t kBackspace = 8;
    constexpr std::uint32_t kFirstPrintable = 32;
    constexpr std::uint32_t kDelete = 127;

    if (unicode == kBackspace) {
        if (typedName_.empty()) {
            return;
        }
        typedName_.pop_back();
    } else if (unicode >= kFirstPrintable && unicode < kDelete) {
        // Printable ASCII only. HabitName rejects control characters anyway, and
        // the tab that separates fields on disk is one of them.
        if (typedName_.size() >= HabitName::kMaxLength) {
            return;
        }
        typedName_.push_back(static_cast<char>(unicode));
    } else {
        return;
    }
    notice_.clear();
    refreshPrompt();
}

void App::onMousePressed(sf::Vector2f point, sf::Mouse::Button button) {
    switch (nav_.hitTest(point)) {
        case view::NavBar::Action::Previous: stepMonth(-1); return;
        case view::NavBar::Action::Next: stepMonth(1); return;
        case view::NavBar::Action::AddHabit: beginAddHabit(); return;
        case view::NavBar::Action::None: break;
    }

    if (const std::optional<std::size_t> row = grid_.removeHitTest(point)) {
        beginRemoveHabit(*row);
        return;
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
    // While a habit is being named or a removal confirmed, the only keys that
    // mean anything are Enter and Esc, and the mouse is inert: a click meant for
    // the question on screen must never land on the grid behind it.
    if (mode_ != Mode::Marking) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        } else if (event.type == sf::Event::TextEntered) {
            onTextEntered(event.text.unicode);
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter) {
                commitEdit();
            } else if (event.key.code == sf::Keyboard::Escape) {
                cancelEdit();
            }
        }
        return;
    }

    switch (event.type) {
        case sf::Event::Closed:
            window_.close();
            break;

        case sf::Event::MouseButtonPressed:
            onMousePressed(window_.mapPixelToCoords(
                               {event.mouseButton.x, event.mouseButton.y}),
                           event.mouseButton.button);
            break;

        case sf::Event::MouseMoved: {
            const sf::Vector2f point =
                window_.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
            nav_.updateHover(point);
            grid_.updateHover(point);
            break;
        }

        case sf::Event::KeyPressed:
            switch (event.key.code) {
                case sf::Keyboard::Left: stepMonth(-1); break;
                case sf::Keyboard::Right: stepMonth(1); break;
                case sf::Keyboard::Escape: window_.close(); break;
                case sf::Keyboard::A:
                    beginAddHabit();
                    swallowNextText_ = true;
                    break;
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
    // The prompt takes the navigation bar's place rather than adding a row, so
    // the window does not jump in height when an edit starts.
    if (mode_ == Mode::Marking) {
        nav_.draw(window_);
    } else {
        prompt_.draw(window_);
    }
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
