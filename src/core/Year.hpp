#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "core/Date.hpp"
#include "core/MonthSheet.hpp"

namespace ht {

/// A full year of habit data: twelve sheets sharing one habit list.
///
/// Invariants, checked on construction and preserved by every mutator:
///   - months()[i].month() == monthFromIndex(i), for all i in [0, 12)
///   - every sheet carries number() as its year
///   - every sheet has the same habit names, in the same order
///
/// The third is the important one. The previous format matched habits across
/// months by line position, so reordering one file silently reattached a year of
/// marks to the wrong habits. Here a habit exists in all twelve months or in
/// none, and addHabit()/removeHabit() are the only ways to change that.
class Year final {
public:
    /// A year with no habits.
    explicit Year(int number);

    /// A year with one unmarked track per name in every month.
    /// Throws std::invalid_argument on a duplicate name.
    Year(int number, const std::vector<HabitName>& names);

    /// A year built from existing sheets.
    /// Throws std::invalid_argument if the sheets are not aligned as above.
    Year(int number, std::array<MonthSheet, kMonthsPerYear> months);

    [[nodiscard]] int number() const noexcept { return number_; }
    [[nodiscard]] const MonthSheet& month(Month month) const noexcept {
        return months_[monthIndex(month)];
    }
    [[nodiscard]] MonthSheet& month(Month month) noexcept {
        return months_[monthIndex(month)];
    }
    [[nodiscard]] const std::array<MonthSheet, kMonthsPerYear>& months()
        const noexcept {
        return months_;
    }

    [[nodiscard]] std::size_t habitCount() const noexcept {
        return months_.front().habitCount();
    }
    [[nodiscard]] std::vector<HabitName> habitNames() const {
        return months_.front().habitNames();
    }
    [[nodiscard]] std::optional<std::size_t> indexOf(const HabitName& name) const {
        return months_.front().indexOf(name);
    }
    [[nodiscard]] bool empty() const noexcept { return habitCount() == 0; }

    /// Adds the habit to all twelve months at once, keeping them aligned.
    /// Throws std::invalid_argument if the name is already tracked.
    void addHabit(const HabitName& name);

    /// Removes the habit from all twelve months. Returns false if not tracked.
    bool removeHabit(const HabitName& name);

    /// Applies `requested` to one day, per toggledMark(). Returns the resulting
    /// mark. Throws std::out_of_range if the habit or day does not exist.
    Mark toggle(Month month, std::size_t habitIndex, unsigned day, Mark requested);

    [[nodiscard]] bool anyMarked() const noexcept;

    [[nodiscard]] friend bool operator==(const Year&, const Year&) = default;

private:
    static std::array<MonthSheet, kMonthsPerYear> buildMonths(
        int number, const std::vector<HabitName>& names);
    void requireAligned() const;

    int number_;
    std::array<MonthSheet, kMonthsPerYear> months_;
};

}  // namespace ht
