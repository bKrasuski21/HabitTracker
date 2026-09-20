#include "core/Year.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace ht {
namespace {

/// std::array has no default constructor for MonthSheet, so the twelve sheets
/// are built explicitly. Kept in one place so the index/Month pairing is stated
/// exactly once.
template <std::size_t... Indices>
std::array<MonthSheet, kMonthsPerYear> makeMonths(int number,
                                                  const std::vector<HabitName>& names,
                                                  std::index_sequence<Indices...>) {
    return {MonthSheet(number, static_cast<Month>(static_cast<unsigned>(Indices)),
                       names)...};
}

}  // namespace

std::array<MonthSheet, kMonthsPerYear> Year::buildMonths(
    int number, const std::vector<HabitName>& names) {
    return makeMonths(number, names, std::make_index_sequence<kMonthsPerYear>{});
}

Year::Year(int number) : Year(number, std::vector<HabitName>{}) {}

Year::Year(int number, const std::vector<HabitName>& names)
    : number_(number), months_(buildMonths(number, names)) {
    if (!isYearInRange(number)) {
        throw std::out_of_range("year " + std::to_string(number) + " is out of range");
    }
    requireAligned();
}

Year::Year(int number, std::array<MonthSheet, kMonthsPerYear> months)
    : number_(number), months_(std::move(months)) {
    if (!isYearInRange(number)) {
        throw std::out_of_range("year " + std::to_string(number) + " is out of range");
    }
    requireAligned();
}

void Year::requireAligned() const {
    const std::vector<HabitName> reference = months_.front().habitNames();
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const MonthSheet& sheet = months_[i];
        if (sheet.month() != monthFromIndex(i)) {
            throw std::invalid_argument("month sheet " + std::to_string(i) +
                                        " is out of order");
        }
        if (sheet.year() != number_) {
            throw std::invalid_argument("month sheet " + std::to_string(i) +
                                        " belongs to a different year");
        }
        if (sheet.habitNames() != reference) {
            throw std::invalid_argument(
                "month sheet " + std::to_string(i) +
                " does not track the same habits as January");
        }
    }
}

void Year::addHabit(const HabitName& name) {
    if (indexOf(name).has_value()) {
        throw std::invalid_argument("duplicate habit name: " + name.str());
    }
    for (MonthSheet& sheet : months_) {
        sheet.addHabit(name);
    }
}

bool Year::removeHabit(const HabitName& name) {
    if (!indexOf(name).has_value()) {
        return false;
    }
    for (MonthSheet& sheet : months_) {
        sheet.removeHabit(name);
    }
    return true;
}

Mark Year::toggle(Month month, std::size_t habitIndex, unsigned day, Mark requested) {
    return months_[monthIndex(month)].track(habitIndex).toggle(day, requested);
}

bool Year::anyMarked() const noexcept {
    for (const MonthSheet& sheet : months_) {
        for (const HabitTrack& track : sheet.tracks()) {
            if (track.anyMarked()) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace ht
