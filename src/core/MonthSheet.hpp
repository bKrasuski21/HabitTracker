#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "core/Date.hpp"
#include "core/HabitTrack.hpp"

namespace ht {

/// Every tracked habit for one month of one year.
///
/// Invariants, all established by the constructors and preserved by every
/// mutator:
///   - isYearInRange(year())
///   - every track has exactly daysInMonth(year(), month()) days
///   - no two tracks share a name
class MonthSheet final {
public:
    /// An unmarked sheet with one track per name, in the order given.
    /// Throws std::invalid_argument on a duplicate name, std::out_of_range on
    /// an out-of-range year.
    MonthSheet(int year, Month month, const std::vector<HabitName>& names);

    /// A sheet holding `tracks`. Any track whose length disagrees with the month
    /// is resized (padded with Mark::None, or truncated) rather than rejected,
    /// so a February sheet saved in a common year loads cleanly into a leap one.
    /// Throws std::invalid_argument on a duplicate name.
    MonthSheet(int year, Month month, std::vector<HabitTrack> tracks);

    [[nodiscard]] int year() const noexcept { return year_; }
    [[nodiscard]] Month month() const noexcept { return month_; }
    [[nodiscard]] unsigned dayCount() const { return daysInMonth(year_, month_); }
    [[nodiscard]] std::size_t habitCount() const noexcept { return tracks_.size(); }
    [[nodiscard]] const std::vector<HabitTrack>& tracks() const noexcept {
        return tracks_;
    }

    /// Throws std::out_of_range unless `index` < habitCount().
    [[nodiscard]] const HabitTrack& track(std::size_t index) const;
    [[nodiscard]] HabitTrack& track(std::size_t index);

    [[nodiscard]] std::optional<std::size_t> indexOf(const HabitName& name) const;
    [[nodiscard]] std::vector<HabitName> habitNames() const;

    /// Appends an unmarked track. Throws std::invalid_argument if the name is
    /// already present.
    void addHabit(const HabitName& name);

    /// Removes the named track. Returns false if it was not present.
    bool removeHabit(const HabitName& name);

    [[nodiscard]] friend bool operator==(const MonthSheet&,
                                         const MonthSheet&) = default;

private:
    void normalizeTrackLengths();
    void requireNoDuplicateNames() const;

    int year_;
    Month month_;
    std::vector<HabitTrack> tracks_;
};

}  // namespace ht
