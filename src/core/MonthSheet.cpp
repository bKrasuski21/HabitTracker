#include "core/MonthSheet.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace ht {

MonthSheet::MonthSheet(int year, Month month, const std::vector<HabitName>& names)
    : year_(year), month_(month) {
    if (!isYearInRange(year)) {
        throw std::out_of_range("year " + std::to_string(year) + " is out of range");
    }
    const unsigned days = daysInMonth(year_, month_);
    tracks_.reserve(names.size());
    for (const HabitName& name : names) {
        tracks_.emplace_back(name, days);
    }
    requireNoDuplicateNames();
}

MonthSheet::MonthSheet(int year, Month month, std::vector<HabitTrack> tracks)
    : year_(year), month_(month), tracks_(std::move(tracks)) {
    if (!isYearInRange(year)) {
        throw std::out_of_range("year " + std::to_string(year) + " is out of range");
    }
    requireNoDuplicateNames();
    normalizeTrackLengths();
}

void MonthSheet::normalizeTrackLengths() {
    const unsigned days = daysInMonth(year_, month_);
    for (HabitTrack& track : tracks_) {
        if (track.dayCount() != days) {
            track.resizeTo(days);
        }
    }
}

void MonthSheet::requireNoDuplicateNames() const {
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        for (std::size_t j = i + 1; j < tracks_.size(); ++j) {
            if (tracks_[i].name() == tracks_[j].name()) {
                throw std::invalid_argument("duplicate habit name: " +
                                            tracks_[i].name().str());
            }
        }
    }
}

const HabitTrack& MonthSheet::track(std::size_t index) const {
    if (index >= tracks_.size()) {
        throw std::out_of_range("habit index " + std::to_string(index) +
                                " is out of range");
    }
    return tracks_[index];
}

HabitTrack& MonthSheet::track(std::size_t index) {
    if (index >= tracks_.size()) {
        throw std::out_of_range("habit index " + std::to_string(index) +
                                " is out of range");
    }
    return tracks_[index];
}

std::optional<std::size_t> MonthSheet::indexOf(const HabitName& name) const {
    const auto it = std::find_if(
        tracks_.begin(), tracks_.end(),
        [&name](const HabitTrack& track) { return track.name() == name; });
    if (it == tracks_.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(tracks_.begin(), it));
}

std::vector<HabitName> MonthSheet::habitNames() const {
    std::vector<HabitName> names;
    names.reserve(tracks_.size());
    for (const HabitTrack& track : tracks_) {
        names.push_back(track.name());
    }
    return names;
}

void MonthSheet::addHabit(const HabitName& name) {
    if (indexOf(name).has_value()) {
        throw std::invalid_argument("duplicate habit name: " + name.str());
    }
    tracks_.emplace_back(name, daysInMonth(year_, month_));
}

bool MonthSheet::removeHabit(const HabitName& name) {
    const std::optional<std::size_t> index = indexOf(name);
    if (!index.has_value()) {
        return false;
    }
    tracks_.erase(tracks_.begin() + static_cast<std::ptrdiff_t>(*index));
    return true;
}

}  // namespace ht
