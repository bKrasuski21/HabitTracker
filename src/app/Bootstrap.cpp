#include "app/Bootstrap.hpp"

#include <charconv>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

#include "core/HabitName.hpp"

namespace ht::bootstrap {
namespace {

/// Reads one line. Returns nullopt at end of input, so every prompt can tell
/// "the user pressed Ctrl-D" apart from "the user typed nothing".
[[nodiscard]] std::optional<std::string> readLine(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::nullopt;
    }
    return line;
}

[[nodiscard]] std::optional<std::size_t> parseCount(std::string_view text) {
    const std::string_view value = trimmed(text);
    std::size_t parsed = 0;
    const char* const begin = value.data();
    const char* const end = begin + value.size();
    const std::from_chars_result result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::optional<std::size_t> askHabitCount(std::istream& in,
                                                       std::ostream& out) {
    while (true) {
        out << "How many habits would you like to track? (1-" << kMaxHabits
            << ", or press Enter to quit): " << std::flush;
        const std::optional<std::string> line = readLine(in);
        if (!line.has_value() || trimmed(*line).empty()) {
            return std::nullopt;
        }
        const std::optional<std::size_t> count = parseCount(*line);
        if (!count.has_value() || *count == 0 || *count > kMaxHabits) {
            out << "  Please enter a whole number between 1 and " << kMaxHabits << ".\n";
            continue;
        }
        return count;
    }
}

/// Asks for habit number `ordinal` of `total`, re-prompting until the answer is
/// a valid name that is not already taken.
[[nodiscard]] std::optional<HabitName> askHabitName(const Year& year, std::size_t ordinal,
                                                    std::size_t total, std::istream& in,
                                                    std::ostream& out) {
    while (true) {
        out << "  Habit " << ordinal << " of " << total << ": " << std::flush;
        const std::optional<std::string> line = readLine(in);
        if (!line.has_value()) {
            return std::nullopt;
        }
        const std::optional<HabitName> name = HabitName::tryMake(*line);
        if (!name.has_value()) {
            out << "    " << HabitName::rejectionReason(*line) << ". Try again.\n";
            continue;
        }
        if (year.indexOf(*name).has_value()) {
            out << "    \"" << name->str() << "\" is already on the list. Try again.\n";
            continue;
        }
        return name;
    }
}

}  // namespace

bool populate(Year& year, std::istream& in, std::ostream& out) {
    out << "No habits are set up for " << year.number() << " yet.\n"
        << "Whole names are fine -- \"read for 20 minutes\" is one habit.\n\n";

    const std::optional<std::size_t> count = askHabitCount(in, out);
    if (!count.has_value()) {
        return false;
    }

    for (std::size_t i = 0; i < *count; ++i) {
        const std::optional<HabitName> name = askHabitName(year, i + 1, *count, in, out);
        if (!name.has_value()) {
            break;  // End of input: keep whatever was entered.
        }
        year.addHabit(*name);
    }

    if (year.empty()) {
        return false;
    }
    out << "\nTracking " << year.habitCount() << " habit"
        << (year.habitCount() == 1 ? "" : "s") << " for " << year.number() << ".\n";
    return true;
}

}  // namespace ht::bootstrap
