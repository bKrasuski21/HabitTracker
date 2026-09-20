#include "core/LegacyImport.hpp"

#include <fstream>
#include <map>
#include <sstream>
#include <system_error>
#include <utility>

#include "core/Date.hpp"
#include "core/HabitName.hpp"
#include "core/HabitTrack.hpp"

namespace ht::legacy {
namespace {

[[nodiscard]] std::optional<std::string> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        return std::nullopt;
    }
    return buffer.str();
}

}  // namespace

std::filesystem::path legacyFilePath(const std::filesystem::path& dataDirectory,
                                     std::size_t monthIndex) {
    return dataDirectory / ("month" + std::to_string(monthIndex));
}

bool hasLegacyData(const std::filesystem::path& dataDirectory) {
    std::error_code ec;
    for (std::size_t i = 0; i < kLegacyFileCount; ++i) {
        if (std::filesystem::exists(legacyFilePath(dataDirectory, i), ec)) {
            return true;
        }
    }
    return false;
}

std::optional<std::pair<std::string_view, std::string_view>> splitLegacyLine(
    std::string_view line) {
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    const std::size_t comma = line.rfind(',');
    if (comma == std::string_view::npos) {
        return std::nullopt;
    }
    return std::pair{line.substr(0, comma), line.substr(comma + 1)};
}

ImportResult importYear(const std::filesystem::path& dataDirectory, int yearNumber) {
    ImportResult result;
    if (!isYearInRange(yearNumber)) {
        result.error = "year " + std::to_string(yearNumber) + " is out of range";
        return result;
    }

    // Habit name -> its marks per month, keyed by name so a file that lists
    // habits in a different order still lands on the right rows.
    std::vector<HabitName> order;
    std::map<std::string, std::array<std::string, kMonthsPerYear>> byName;

    for (std::size_t m = 0; m < kLegacyFileCount; ++m) {
        const std::filesystem::path path = legacyFilePath(dataDirectory, m);
        const std::optional<std::string> contents = readFile(path);
        if (!contents.has_value()) {
            result.warnings.push_back("could not read " + path.filename().string() +
                                      "; that month was imported unmarked");
            continue;
        }

        std::istringstream lines(*contents);
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(lines, line)) {
            ++lineNumber;
            if (trimmed(line).empty()) {
                continue;
            }
            const auto split = splitLegacyLine(line);
            if (!split.has_value()) {
                result.warnings.push_back(path.filename().string() + " line " +
                                          std::to_string(lineNumber) +
                                          " has no comma; it was skipped");
                continue;
            }
            const std::optional<HabitName> name = HabitName::tryMake(split->first);
            if (!name.has_value()) {
                result.warnings.push_back(path.filename().string() + " line " +
                                          std::to_string(lineNumber) + ": " +
                                          HabitName::rejectionReason(split->first) +
                                          "; it was skipped");
                continue;
            }
            auto [entry, inserted] = byName.try_emplace(name->str());
            if (inserted) {
                order.push_back(*name);
            }
            if (!entry->second[m].empty()) {
                result.warnings.push_back("habit \"" + name->str() + "\" appears twice in " +
                                          path.filename().string() +
                                          "; the first entry was kept");
                continue;
            }
            entry->second[m] = std::string(split->second);
        }
    }

    if (order.empty()) {
        result.error = "no habits could be read from the old data files";
        return result;
    }

    std::array<std::vector<HabitTrack>, kMonthsPerYear> tracks;
    for (std::size_t m = 0; m < kMonthsPerYear; ++m) {
        const Month month = monthFromIndex(m);
        const unsigned days = daysInMonth(yearNumber, month);
        for (const HabitName& name : order) {
            const std::string& encoded = byName[name.str()][m];
            if (encoded.empty()) {
                result.warnings.push_back("habit \"" + name.str() + "\" had no data for " +
                                          std::string(monthName(month)) +
                                          "; it was imported unmarked");
            }
            std::vector<Mark> marks;
            marks.reserve(days);
            for (unsigned day = 0; day < days; ++day) {
                if (day >= encoded.size()) {
                    marks.push_back(Mark::None);
                    continue;
                }
                marks.push_back(markFromChar(encoded[day]).value_or(Mark::None));
            }
            tracks[m].emplace_back(name, std::move(marks));
        }
    }

    try {
        std::array<MonthSheet, kMonthsPerYear> sheets{
            MonthSheet(yearNumber, Month::January, std::move(tracks[0])),
            MonthSheet(yearNumber, Month::February, std::move(tracks[1])),
            MonthSheet(yearNumber, Month::March, std::move(tracks[2])),
            MonthSheet(yearNumber, Month::April, std::move(tracks[3])),
            MonthSheet(yearNumber, Month::May, std::move(tracks[4])),
            MonthSheet(yearNumber, Month::June, std::move(tracks[5])),
            MonthSheet(yearNumber, Month::July, std::move(tracks[6])),
            MonthSheet(yearNumber, Month::August, std::move(tracks[7])),
            MonthSheet(yearNumber, Month::September, std::move(tracks[8])),
            MonthSheet(yearNumber, Month::October, std::move(tracks[9])),
            MonthSheet(yearNumber, Month::November, std::move(tracks[10])),
            MonthSheet(yearNumber, Month::December, std::move(tracks[11]))};
        result.year.emplace(yearNumber, std::move(sheets));
    } catch (const std::exception& e) {
        result.error = e.what();
        result.year.reset();
    }
    return result;
}

}  // namespace ht::legacy
