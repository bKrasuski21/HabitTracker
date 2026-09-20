#include "core/HabitTrack.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "core/Date.hpp"

namespace ht {

void HabitTrack::requireValidDayCount(std::size_t dayCount) {
    if (dayCount < kMinDaysInMonth || dayCount > kMaxDaysInMonth) {
        throw std::out_of_range("day count " + std::to_string(dayCount) +
                                " is not a possible month length");
    }
}

HabitTrack::HabitTrack(HabitName name, unsigned dayCount)
    : name_(std::move(name)), marks_(dayCount, Mark::None) {
    requireValidDayCount(dayCount);
}

HabitTrack::HabitTrack(HabitName name, std::vector<Mark> marks)
    : name_(std::move(name)), marks_(std::move(marks)) {
    requireValidDayCount(marks_.size());
}

Mark HabitTrack::mark(unsigned day) const {
    if (day >= dayCount()) {
        throw std::out_of_range("day " + std::to_string(day) + " is outside the month");
    }
    return marks_[day];
}

void HabitTrack::setMark(unsigned day, Mark mark) {
    if (day >= dayCount()) {
        throw std::out_of_range("day " + std::to_string(day) + " is outside the month");
    }
    marks_[day] = mark;
}

Mark HabitTrack::toggle(unsigned day, Mark requested) {
    const Mark next = toggledMark(mark(day), requested);
    setMark(day, next);
    return next;
}

void HabitTrack::resizeTo(unsigned dayCount) {
    requireValidDayCount(dayCount);
    marks_.resize(dayCount, Mark::None);
}

bool HabitTrack::anyMarked() const noexcept {
    return std::any_of(marks_.begin(), marks_.end(),
                       [](Mark mark) { return mark != Mark::None; });
}

}  // namespace ht
