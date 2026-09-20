#pragma once

#include <cstddef>
#include <vector>

#include "core/HabitName.hpp"
#include "core/Mark.hpp"

namespace ht {

/// One habit's marks for one month.
///
/// Invariant: marks().size() == dayCount(), and dayCount() is in
/// [kMinDaysInMonth, kMaxDaysInMonth]. Every constructor establishes it and no
/// member can break it, so a track can never disagree with its own length.
class HabitTrack final {
public:
    /// An unmarked track of `dayCount` days.
    /// Throws std::out_of_range if `dayCount` is not a possible month length.
    HabitTrack(HabitName name, unsigned dayCount);

    /// A track holding `marks`, one per day.
    /// Throws std::out_of_range if marks.size() is not a possible month length.
    HabitTrack(HabitName name, std::vector<Mark> marks);

    [[nodiscard]] const HabitName& name() const noexcept { return name_; }
    [[nodiscard]] unsigned dayCount() const noexcept {
        return static_cast<unsigned>(marks_.size());
    }
    [[nodiscard]] const std::vector<Mark>& marks() const noexcept { return marks_; }

    /// `day` is zero-based. Throws std::out_of_range if it is not a day of this
    /// month -- there is no unchecked accessor by design.
    [[nodiscard]] Mark mark(unsigned day) const;
    void setMark(unsigned day, Mark mark);

    /// Applies `requested` to `day` per toggledMark(). Returns the resulting
    /// mark. Throws std::out_of_range if `day` is out of range.
    Mark toggle(unsigned day, Mark requested);

    /// Grows or shrinks to `dayCount` days, preserving existing marks and
    /// padding with Mark::None. Used when a sheet is loaded for a year whose
    /// February differs in length from the year it was saved under.
    /// Throws std::out_of_range if `dayCount` is not a possible month length.
    void resizeTo(unsigned dayCount);

    [[nodiscard]] bool anyMarked() const noexcept;

    [[nodiscard]] friend bool operator==(const HabitTrack&,
                                         const HabitTrack&) = default;

private:
    static void requireValidDayCount(std::size_t dayCount);

    HabitName name_;
    std::vector<Mark> marks_;
};

}  // namespace ht
