#include <charconv>
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
        << "  --month <1-12>     Month to open. Default: the current month.\n"
        << "  --help             Show this message.\n\n"
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
    bool showHelp = false;
};

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
    if (options.config.fontPath.empty()) {
        std::cerr << "HabitTracker: could not find " << kDefaultFontName
                  << " beside the executable or in the current directory.\n"
                  << "Pass --font <path> to point at one.\n";
        return std::nullopt;
    }
    return options;
}

int runApplication(const ht::App::Config& config) {
    // The store outlives the App on purpose: its destructor is the last chance
    // to flush, and it runs after the window is gone.
    ht::HabitStore store(config.dataDirectory, config.year);

    for (const std::string& message : store.loadMessages()) {
        std::cerr << "HabitTracker: " << message << '\n';
    }
    if (store.importedLegacyData()) {
        std::cout << "Imported your old month files into " << store.filePath().string()
                  << ".\nThe originals were left where they are.\n";
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

    ht::App app(config, store);
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
        return runApplication(options->config);
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
