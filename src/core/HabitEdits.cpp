#include "core/HabitEdits.hpp"

#include <algorithm>
#include <optional>

#include "core/Mark.hpp"
#include "core/MonthSheet.hpp"

namespace ht {
namespace {

[[nodiscard]] std::string quote(const HabitName& name) {
    return '"' + name.str() + '"';
}

[[nodiscard]] std::string plural(std::size_t count, const std::string& noun) {
    return std::to_string(count) + ' ' + noun + (count == 1 ? "" : "s");
}

}  // namespace

std::size_t markedDayCount(const Year& year, const HabitName& name) {
    const std::optional<std::size_t> index = year.indexOf(name);
    if (!index.has_value()) {
        return 0;
    }

    std::size_t total = 0;
    for (const MonthSheet& sheet : year.months()) {
        for (const Mark mark : sheet.track(*index).marks()) {
            if (mark != Mark::None) {
                ++total;
            }
        }
    }
    return total;
}

HabitEditReport planHabitEdits(const Year& year, const std::vector<HabitEdit>& edits) {
    HabitEditReport report;

    // The habit list as it would stand after each edit, so a batch is judged in
    // sequence rather than every edit being judged against the starting year.
    std::vector<HabitName> pending = year.habitNames();

    for (const HabitEdit& edit : edits) {
        const auto found = std::find(pending.begin(), pending.end(), edit.name);
        const bool tracked = found != pending.end();
        // Distinguishes "the year already had it" from "this batch named it
        // twice": exactly one of those two is true whenever an edit is refused.
        const bool inStartingYear = year.indexOf(edit.name).has_value();

        if (edit.kind == HabitEdit::Kind::Add) {
            if (tracked) {
                report.problems.push_back(quote(edit.name) +
                                          (inStartingYear
                                               ? " is already tracked"
                                               : " was given twice in one command"));
                continue;
            }
            pending.push_back(edit.name);
            report.applied.push_back("added " + quote(edit.name));
            continue;
        }

        if (!tracked) {
            report.problems.push_back(quote(edit.name) +
                                      (inStartingYear
                                           ? " was given twice in one command"
                                           : " is not tracked"));
            continue;
        }
        const std::size_t marks = markedDayCount(year, edit.name);
        report.marksDiscarded += marks;
        pending.erase(found);
        report.applied.push_back("removed " + quote(edit.name) + " and its " +
                                 plural(marks, "marked day"));
    }

    if (!report.ok()) {
        report.applied.clear();
        report.marksDiscarded = 0;
    }
    return report;
}

HabitEditReport applyHabitEdits(Year& year, const std::vector<HabitEdit>& edits) {
    HabitEditReport report = planHabitEdits(year, edits);
    if (!report.ok()) {
        return report;  // The year is untouched.
    }

    // Every edit was checked above, so addHabit() cannot throw and removeHabit()
    // cannot report a miss.
    for (const HabitEdit& edit : edits) {
        if (edit.kind == HabitEdit::Kind::Add) {
            year.addHabit(edit.name);
        } else {
            year.removeHabit(edit.name);
        }
    }
    return report;
}

}  // namespace ht
