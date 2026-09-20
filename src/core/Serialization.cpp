#include "core/Serialization.hpp"

#include <charconv>
#include <utility>

#include "core/Date.hpp"
#include "core/HabitTrack.hpp"

namespace ht::serialize {
namespace {

constexpr std::string_view kYearKey = "year";
constexpr std::string_view kHabitKey = "habit";
/// habit + name + twelve months.
constexpr std::size_t kHabitFieldCount = 2 + kMonthsPerYear;

[[nodiscard]] std::vector<std::string_view> splitFields(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (true) {
        const std::size_t sep = line.find(kFieldSeparator, begin);
        if (sep == std::string_view::npos) {
            fields.push_back(line.substr(begin));
            return fields;
        }
        fields.push_back(line.substr(begin, sep - begin));
        begin = sep + 1;
    }
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

/// A line stripped of a trailing carriage return, so files written on Windows
/// read correctly everywhere.
[[nodiscard]] std::string_view withoutCarriageReturn(std::string_view line) {
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

[[nodiscard]] bool isSkippable(std::string_view line) {
    const std::string_view content = trimmed(line);
    return content.empty() || content.front() == '#';
}

struct Line final {
    std::string_view text;
    std::size_t number = 0;
};

[[nodiscard]] std::vector<Line> significantLines(std::string_view text) {
    std::vector<Line> lines;
    std::size_t begin = 0;
    std::size_t number = 0;
    while (begin <= text.size()) {
        const std::size_t newline = text.find('\n', begin);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        ++number;
        const std::string_view line =
            withoutCarriageReturn(text.substr(begin, end - begin));
        if (!isSkippable(line)) {
            lines.push_back(Line{line, number});
        }
        if (newline == std::string_view::npos) {
            break;
        }
        begin = newline + 1;
    }
    return lines;
}

/// Decodes one month field into exactly `days` marks. Unreadable characters
/// become Mark::None and a short or long field is padded or truncated; each
/// repair appends a warning rather than failing, so one bad byte never costs the
/// user a year of data.
[[nodiscard]] std::vector<Mark> decodeMonth(std::string_view field, unsigned days,
                                            const std::string& habit, Month month,
                                            std::vector<std::string>& warnings) {
    std::vector<Mark> marks;
    marks.reserve(days);
    bool sawBadCharacter = false;
    for (std::size_t day = 0; day < days; ++day) {
        if (day >= field.size()) {
            marks.push_back(Mark::None);
            continue;
        }
        const std::optional<Mark> mark = markFromChar(field[day]);
        if (!mark.has_value()) {
            sawBadCharacter = true;
            marks.push_back(Mark::None);
            continue;
        }
        marks.push_back(*mark);
    }

    const std::string where =
        std::string(monthName(month)) + " of habit \"" + habit + "\"";
    if (field.size() != days) {
        warnings.push_back(where + " held " + std::to_string(field.size()) +
                           " days but the month has " + std::to_string(days) +
                           "; it was adjusted");
    }
    if (sawBadCharacter) {
        warnings.push_back(where + " held unreadable days; they were left unmarked");
    }
    return marks;
}

}  // namespace

std::string toText(const Year& year) {
    std::string out;
    // Header, plus a generous estimate of one line per habit.
    out.reserve(64 + year.habitCount() * (HabitName::kMaxLength + kMaxDaysInMonth *
                                                                     kMonthsPerYear));

    out += kMagic;
    out += kFieldSeparator;
    out += std::to_string(kFormatVersion);
    out += '\n';

    out += kYearKey;
    out += kFieldSeparator;
    out += std::to_string(year.number());
    out += '\n';

    for (std::size_t habit = 0; habit < year.habitCount(); ++habit) {
        out += kHabitKey;
        out += kFieldSeparator;
        out += year.month(Month::January).track(habit).name().str();
        for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
            out += kFieldSeparator;
            for (const Mark mark : year.month(monthFromIndex(i)).track(habit).marks()) {
                out += markToChar(mark);
            }
        }
        out += '\n';
    }
    return out;
}

ParseResult parse(std::string_view text) {
    ParseResult result;
    const std::vector<Line> lines = significantLines(text);

    if (lines.empty()) {
        result.error.emplace(0, "the file is empty");
        return result;
    }

    const std::vector<std::string_view> header = splitFields(lines[0].text);
    if (header.size() != 2 || header[0] != kMagic) {
        result.error.emplace(lines[0].number,
                                  "this is not a HabitTracker file (expected a \"" +
                                      std::string(kMagic) + "\" header)");
        return result;
    }
    const std::optional<int> version = parseInt(header[1]);
    if (!version.has_value()) {
        result.error.emplace(lines[0].number, "the format version is not a number");
        return result;
    }
    if (*version != kFormatVersion) {
        result.error.emplace(
            lines[0].number,
            "unsupported format version " + std::to_string(*version) + " (this build " +
                "reads version " + std::to_string(kFormatVersion) + ")");
        return result;
    }

    if (lines.size() < 2) {
        result.error.emplace(lines[0].number, "the file has no year line");
        return result;
    }
    const std::vector<std::string_view> yearFields = splitFields(lines[1].text);
    if (yearFields.size() != 2 || yearFields[0] != kYearKey) {
        result.error.emplace(lines[1].number,
                                  "expected a \"year\" line after the header");
        return result;
    }
    const std::optional<int> yearNumber = parseInt(yearFields[1]);
    if (!yearNumber.has_value() || !isYearInRange(*yearNumber)) {
        result.error.emplace(lines[1].number,
                                  "\"" + std::string(yearFields[1]) +
                                      "\" is not a year between " +
                                      std::to_string(kMinYear) + " and " +
                                      std::to_string(kMaxYear));
        return result;
    }

    // Tracks are collected per month, then handed to the sheets in one go so the
    // Year constructor can enforce alignment across all twelve at once.
    std::array<std::vector<HabitTrack>, kMonthsPerYear> byMonth;
    std::vector<HabitName> seen;

    for (std::size_t i = 2; i < lines.size(); ++i) {
        const Line& line = lines[i];
        const std::vector<std::string_view> fields = splitFields(line.text);
        if (fields.empty() || fields[0] != kHabitKey) {
            const std::string_view found = fields.empty() ? std::string_view{} : fields[0];
            result.error.emplace(
                line.number,
                "expected a \"habit\" line, found \"" + std::string(found) + "\"");
            return result;
        }
        if (fields.size() != kHabitFieldCount) {
            result.error.emplace(
                line.number, "a habit line needs a name and " +
                                 std::to_string(kMonthsPerYear) + " months, found " +
                                 std::to_string(fields.size() - 1) + " fields");
            return result;
        }

        const std::optional<HabitName> name = HabitName::tryMake(fields[1]);
        if (!name.has_value()) {
            result.error.emplace(
                line.number, "invalid habit name: " + HabitName::rejectionReason(fields[1]));
            return result;
        }
        for (const HabitName& earlier : seen) {
            if (earlier == *name) {
                result.error.emplace(
                    line.number,
                    "habit \"" + name->str() + "\" appears more than once");
                return result;
            }
        }
        seen.push_back(*name);

        for (std::size_t m = 0; m < kMonthsPerYear; ++m) {
            const Month month = monthFromIndex(m);
            const unsigned days = daysInMonth(*yearNumber, month);
            byMonth[m].emplace_back(*name, decodeMonth(fields[2 + m], days, name->str(),
                                                       month, result.warnings));
        }
    }

    try {
        std::array<MonthSheet, kMonthsPerYear> sheets{
            MonthSheet(*yearNumber, Month::January, std::move(byMonth[0])),
            MonthSheet(*yearNumber, Month::February, std::move(byMonth[1])),
            MonthSheet(*yearNumber, Month::March, std::move(byMonth[2])),
            MonthSheet(*yearNumber, Month::April, std::move(byMonth[3])),
            MonthSheet(*yearNumber, Month::May, std::move(byMonth[4])),
            MonthSheet(*yearNumber, Month::June, std::move(byMonth[5])),
            MonthSheet(*yearNumber, Month::July, std::move(byMonth[6])),
            MonthSheet(*yearNumber, Month::August, std::move(byMonth[7])),
            MonthSheet(*yearNumber, Month::September, std::move(byMonth[8])),
            MonthSheet(*yearNumber, Month::October, std::move(byMonth[9])),
            MonthSheet(*yearNumber, Month::November, std::move(byMonth[10])),
            MonthSheet(*yearNumber, Month::December, std::move(byMonth[11]))};
        result.year.emplace(*yearNumber, std::move(sheets));
    } catch (const std::exception& e) {
        // The model refused the data; report it instead of storing something
        // that violates its own invariants.
        result.error.emplace(0, e.what());
        result.year.reset();
    }
    return result;
}

}  // namespace ht::serialize
