#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace ht {

/// Calendar month, zero-based so it indexes directly into a 12-element array.
///
/// Zero-based is deliberate: the previous design stored a one-based int and
/// subtracted one at every use, which is an off-by-one waiting to happen.
enum class Month : unsigned {
    January = 0,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December,
};

inline constexpr std::size_t kMonthsPerYear = 12;

/// Bounds on a representable year. Narrow enough to catch a garbled file,
/// wide enough that nobody will ever hit it in practice.
inline constexpr int kMinYear = 1970;
inline constexpr int kMaxYear = 9999;

/// Every month has at least 28 and at most 31 days; used to bound day counts.
inline constexpr unsigned kMinDaysInMonth = 28;
inline constexpr unsigned kMaxDaysInMonth = 31;

[[nodiscard]] constexpr bool isYearInRange(int year) noexcept {
    return year >= kMinYear && year <= kMaxYear;
}

[[nodiscard]] constexpr std::size_t monthIndex(Month month) noexcept {
    return static_cast<std::size_t>(month);
}

/// Throws std::out_of_range unless `index` is in [0, 12).
[[nodiscard]] constexpr Month monthFromIndex(std::size_t index) {
    if (index >= kMonthsPerYear) {
        throw std::out_of_range("month index out of range");
    }
    return static_cast<Month>(index);
}

[[nodiscard]] constexpr bool isLeapYear(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/// Number of days in `month` of `year`, leap years included.
///
/// Precondition: isYearInRange(year). Throws std::out_of_range otherwise, so a
/// corrupt year can never silently produce a wrong-length sheet.
[[nodiscard]] constexpr unsigned daysInMonth(int year, Month month) {
    if (!isYearInRange(year)) {
        throw std::out_of_range("year out of range");
    }
    constexpr std::array<unsigned, kMonthsPerYear> kDays{31, 28, 31, 30, 31, 30,
                                                         31, 31, 30, 31, 30, 31};
    const unsigned days = kDays[monthIndex(month)];
    if (month == Month::February && isLeapYear(year)) {
        return days + 1;
    }
    return days;
}

[[nodiscard]] constexpr std::string_view monthName(Month month) noexcept {
    constexpr std::array<std::string_view, kMonthsPerYear> kNames{
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December"};
    return kNames[monthIndex(month)];
}

/// Month arithmetic that wraps at the year boundary. `step` may be negative and
/// of any magnitude; the result is always a valid Month.
[[nodiscard]] constexpr Month monthStep(Month month, int step) noexcept {
    constexpr int kMonths = static_cast<int>(kMonthsPerYear);
    int index = static_cast<int>(monthIndex(month)) + step % kMonths;
    index = (index % kMonths + kMonths) % kMonths;
    return static_cast<Month>(static_cast<unsigned>(index));
}

struct YearMonth final {
    int year = kMinYear;
    Month month = Month::January;

    [[nodiscard]] friend constexpr bool operator==(const YearMonth&,
                                                   const YearMonth&) = default;
};

struct YearMonthDay final {
    int year = kMinYear;
    Month month = Month::January;
    unsigned day = 0;  ///< Zero-based day of the month.

    [[nodiscard]] constexpr YearMonth yearMonth() const noexcept {
        return YearMonth{year, month};
    }

    [[nodiscard]] friend constexpr bool operator==(const YearMonthDay&,
                                                   const YearMonthDay&) = default;
};

/// Today from the system clock. Never throws; falls back to 1 January kMinYear
/// if the clock is unusable, which is a valid date and so cannot break callers.
[[nodiscard]] YearMonthDay currentDate() noexcept;

/// Today's year and month.
[[nodiscard]] inline YearMonth currentYearMonth() noexcept {
    return currentDate().yearMonth();
}

}  // namespace ht
