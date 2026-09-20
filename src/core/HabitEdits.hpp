#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/HabitName.hpp"
#include "core/Year.hpp"

namespace ht {

/// One requested change to a year's habit list.
struct HabitEdit final {
    enum class Kind : std::uint8_t { Add, Remove };

    Kind kind;
    HabitName name;
};

/// What a batch of edits did, or would do.
///
/// A batch is all-or-nothing: `problems` non-empty means the year was left
/// exactly as it was, so a typo in the fourth name cannot leave the first three
/// half-applied.
struct HabitEditReport final {
    /// One line per edit, in order, saying what happened. Empty when refused.
    std::vector<std::string> applied;
    /// Every reason the batch was refused, not just the first.
    std::vector<std::string> problems;
    /// Marked days the removals erase. Zero unless something is removed.
    std::size_t marksDiscarded = 0;

    [[nodiscard]] bool ok() const noexcept { return problems.empty(); }
};

/// How many days `name` has marked across all twelve months. Zero if `name` is
/// not tracked. This is what a removal destroys, so the caller can say so before
/// doing it.
[[nodiscard]] std::size_t markedDayCount(const Year& year, const HabitName& name);

/// Checks `edits` against `year` without modifying anything.
///
/// Edits are validated in sequence, so adding a name and removing it again in
/// one batch is accepted, while naming the same habit twice is not.
[[nodiscard]] HabitEditReport planHabitEdits(const Year& year,
                                             const std::vector<HabitEdit>& edits);

/// Applies `edits` to `year` if every one of them is valid, and otherwise
/// changes nothing. The returned report is the one planHabitEdits() gives.
HabitEditReport applyHabitEdits(Year& year, const std::vector<HabitEdit>& edits);

}  // namespace ht
