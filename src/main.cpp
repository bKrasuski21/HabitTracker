#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/App.hpp"
#include "app/Bootstrap.hpp"
#include "core/Date.hpp"
#include "core/HabitEdits.hpp"
#include "core/HabitName.hpp"
#include "core/HabitStore.hpp"
#include "core/Paths.hpp"

namespace {

constexpr std::string_view kDefaultFontName = "font.ttf";

void printUsage(std::ostream& out) {
    out << "HabitTracker -- a year of habits, one square per day.\n\n"
        << "Usage: HabitTracker [options]\n\n"
        << "  --data-dir <path>  Where habit files live.\n"
        << "                     Default: $HABITTRACKER_DATA_DIR, else \"data\" beside\n"
        << "                     the executable.\n"
        << "  --font <path>      TrueType font to render with.\n"
        << "                     Default: font.ttf beside the executable.\n"
        << "  --year <number>    Year to open. Default: the current year.\n"
        << "  --month <1-12>     Month to open. Default: the current month.\n\n"
        << "  --add-habit <name>     Start tracking a habit. May be repeated.\n"
        << "  --remove-habit <name>  Stop tracking a habit and erase its marks.\n"
        << "                         May be repeated. Asks first.\n"
        << "  --list-habits          Print this year's habits.\n"
        << "  --yes, -y              Answer yes to the --remove-habit prompt.\n"
        << "                         These four report and exit without opening\n"
        << "                         the window; habits can also be added and\n"
        << "                         removed inside the app.\n"
        << "  --help                 Show this message.\n\n"
        << "Left click marks a day done, right click marks it partial, and clicking\n"
        << "the same mark again clears it. Left and Right arrows change month.\n"
        << "Changes are saved automatically.\n";
}

[[nodiscard]] std::optional<int> parseInt(std::string_view text) {
    int value = 0;
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

struct Options final {
    ht::App::Config config;
    /// --add-habit and --remove-habit, in the order given: the order is what
    /// makes "remove x, add x" mean something different from "add x, remove x".
    std::vector<ht::HabitEdit> habitEdits;
    bool listHabits = false;
    bool assumeYes = false;
    bool showHelp = false;
};

/// Parses a habit name from the command line, reporting why it was rejected.
[[nodiscard]] std::optional<ht::HabitName> parseHabitName(std::string_view text,
                                                          std::string_view flag) {
    std::optional<ht::HabitName> name = ht::HabitName::tryMake(text);
    if (!name.has_value()) {
        std::cerr << "HabitTracker: " << flag << " \"" << text
                  << "\": " << ht::HabitName::rejectionReason(text) << ".\n";
    }
    return name;
}

/// Parses the command line. Returns nullopt after reporting a usage error.
[[nodiscard]] std::optional<Options> parseArguments(const std::vector<std::string_view>& args) {
    const ht::YearMonthDay today = ht::currentDate();
    Options options;
    options.config.dataDirectory = ht::paths::defaultDataDirectory();
    options.config.year = today.year;
    options.config.startMonth = today.month;

    std::optional<std::filesystem::path> fontOverride;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        const auto next = [&](std::string_view name) -> std::optional<std::string_view> {
            if (i + 1 >= args.size()) {
                std::cerr << "HabitTracker: " << name << " needs a value.\n";
                return std::nullopt;
            }
            return args[++i];
        };

        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
            return options;
        }
        if (arg == "--data-dir") {
            const std::optional<std::string_view> value = next(arg);
            if (!value.has_value()) return std::nullopt;
            options.config.dataDirectory = std::filesystem::path(*value);
        } else if (arg == "--font") {
            const std::optional<std::string_view> value = next(arg);
            if (!value.has_value()) return std::nullopt;
            fontOverride = std::filesystem::path(*value);
        } else if (arg == "--year") {
            const std::optional<std::string_view> value = next(arg);
            if (!value.has_value()) return std::nullopt;
            const std::optional<int> year = parseInt(*value);
            if (!year.has_value() || !ht::isYearInRange(*year)) {
                std::cerr << "HabitTracker: --year must be between " << ht::kMinYear
                          << " and " << ht::kMaxYear << ".\n";
                return std::nullopt;
            }
            options.config.year = *year;
        } else if (arg == "--month") {
            const std::optional<std::string_view> value = next(arg);
            if (!value.has_value()) return std::nullopt;
            const std::optional<int> month = parseInt(*value);
            if (!month.has_value() || *month < 1 ||
                *month > static_cast<int>(ht::kMonthsPerYear)) {
                std::cerr << "HabitTracker: --month must be between 1 and 12.\n";
                return std::nullopt;
            }
            options.config.startMonth =
                ht::monthFromIndex(static_cast<std::size_t>(*month - 1));
        } else if (arg == "--add-habit" || arg == "--remove-habit") {
            const std::optional<std::string_view> value = next(arg);
            if (!value.has_value()) return std::nullopt;
            const std::optional<ht::HabitName> name = parseHabitName(*value, arg);
            if (!name.has_value()) return std::nullopt;
            options.habitEdits.push_back({arg == "--add-habit"
                                              ? ht::HabitEdit::Kind::Add
                                              : ht::HabitEdit::Kind::Remove,
                                          *name});
        } else if (arg == "--list-habits") {
            options.listHabits = true;
        } else if (arg == "--yes" || arg == "-y") {
            options.assumeYes = true;
        } else {
            std::cerr << "HabitTracker: unknown option \"" << arg << "\".\n\n";
            printUsage(std::cerr);
            return std::nullopt;
        }
    }

    // The year may have been overridden after the start month was defaulted to
    // the current month; that is still a valid month, so nothing to reconcile.
    options.config.fontPath =
        fontOverride.value_or(ht::paths::locateResource(kDefaultFontName));
    if (options.config.fontPath.empty() && !options.listHabits) {
        std::cerr << "HabitTracker: could not find " << kDefaultFontName
                  << " beside the executable or in the current directory.\n"
                  << "Pass --font <path> to point at one.\n";
        return std::nullopt;
    }
    return options;
}

void printHabits(const ht::Year& year, std::ostream& out) {
    if (year.empty()) {
        out << "No habits are tracked for " << year.number() << " yet.\n";
        return;
    }
    out << year.habitCount() << " habit" << (year.habitCount() == 1 ? "" : "s")
        << " tracked for " << year.number() << ":\n";
    for (const ht::HabitName& name : year.habitNames()) {
        const std::size_t marked = ht::markedDayCount(year, name);
        out << "  " << name.str() << "  (" << marked << " marked day"
            << (marked == 1 ? "" : "s") << ")\n";
    }
}

/// Asks before a removal throws marks away. End of input is not consent.
[[nodiscard]] bool confirmDiscard(std::size_t marks, std::istream& in, std::ostream& out) {
    out << "That erases " << marks << " marked day" << (marks == 1 ? "" : "s")
        << ", which cannot be undone.\n"
        << "The file as it stands is kept alongside as a .bak.\n"
        << "Continue? [y/N]: " << std::flush;
    std::string answer;
    if (!std::getline(in, answer)) {
        return false;
    }
    const std::string_view reply = ht::trimmed(answer);
    return reply == "y" || reply == "Y" || reply == "yes" || reply == "Yes";
}

/// Applies --add-habit and --remove-habit and saves the result.
///
/// Returns an exit code if the run should stop here, or nullopt to carry on
/// into the window. The batch is all-or-nothing: on any problem the year is
/// left exactly as it was found.
[[nodiscard]] std::optional<int> editHabits(ht::HabitStore& store, const Options& options) {
    const ht::HabitEditReport plan =
        ht::planHabitEdits(store.year(), options.habitEdits);
    if (!plan.ok()) {
        for (const std::string& problem : plan.problems) {
            std::cerr << "HabitTracker: " << problem << '\n';
        }
        std::cerr << "Nothing was changed. Run --list-habits to see the "
                     "current names.\n";
        return EXIT_FAILURE;
    }

    if (plan.marksDiscarded > 0 && !options.assumeYes &&
        !confirmDiscard(plan.marksDiscarded, std::cin, std::cout)) {
        std::cout << "Nothing was changed.\n";
        return EXIT_SUCCESS;
    }

    const ht::HabitEditReport done =
        ht::applyHabitEdits(store.mutableYear(), options.habitEdits);
    for (const std::string& line : done.applied) {
        std::cout << line << '\n';
    }

    // Written now rather than left to the autosave, so the change survives even
    // if the window never opens.
    if (!store.save()) {
        std::cerr << "HabitTracker: could not save to " << store.filePath().string()
                  << ": " << store.lastError() << '\n';
        return EXIT_FAILURE;
    }
    return std::nullopt;
}

/// The window is sized to its content but capped, and nothing scales the grid to
/// fit, so rows past the cap are drawn where they cannot be seen or clicked. Say
/// so rather than letting them vanish. bootstrap::kMaxHabits is below this cap,
/// so only a file edited by hand or by --add-habit can reach it.
void warnIfCrowded(const ht::Year& year, std::ostream& out) {
    const std::size_t visible = ht::App::maxVisibleHabits();
    if (year.habitCount() > visible) {
        out << "HabitTracker: " << year.habitCount()
            << " habits is more than the window can show (" << visible
            << "); the rows past that are drawn off-screen.\n";
    }
}

int runApplication(const Options& options) {
    // The store outlives the App on purpose: its destructor is the last chance
    // to flush, and it runs after the window is gone.
    ht::HabitStore store(options.config.dataDirectory, options.config.year);

    for (const std::string& message : store.loadMessages()) {
        std::cerr << "HabitTracker: " << message << '\n';
    }
    if (store.importedLegacyData()) {
        std::cout << "Imported your old month files into " << store.filePath().string()
                  << ".\nThe originals were left where they are.\n";
    }

    if (!options.habitEdits.empty()) {
        if (const std::optional<int> code = editHabits(store, options)) {
            return *code;
        }
    }

    warnIfCrowded(store.year(), std::cerr);

    if (options.listHabits) {
        printHabits(store.year(), std::cout);
    }

    // Editing and listing report and exit rather than opening the window, so a
    // run of them can be scripted without leaving a window waiting to be closed.
    if (options.listHabits || !options.habitEdits.empty()) {
        return EXIT_SUCCESS;
    }

    if (store.year().empty()) {
        if (!ht::bootstrap::populate(store.mutableYear(), std::cin, std::cout)) {
            std::cout << "Nothing to track yet -- run again when you know what you "
                         "want to build.\n";
            return EXIT_SUCCESS;
        }
        if (!store.save()) {
            std::cerr << "HabitTracker: could not save to " << store.filePath().string()
                      << ": " << store.lastError() << '\n';
            return EXIT_FAILURE;
        }
    }

    ht::App app(options.config, store);
    return app.run();
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    try {
        const std::optional<Options> options = parseArguments(args);
        if (!options.has_value()) {
            return EXIT_FAILURE;
        }
        if (options->showHelp) {
            printUsage(std::cout);
            return EXIT_SUCCESS;
        }
        return runApplication(*options);
    } catch (const ht::LoadError& e) {
        std::cerr << "HabitTracker: could not read " << e.path().string() << ": "
                  << e.what() << "\n"
                  << "Nothing was written. Move or fix that file and try again.\n";
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "HabitTracker: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "HabitTracker: an unknown error occurred.\n";
        return EXIT_FAILURE;
    }
}
