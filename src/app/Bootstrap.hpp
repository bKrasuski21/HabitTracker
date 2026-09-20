#pragma once

#include <cstddef>
#include <iosfwd>

#include "core/Year.hpp"

namespace ht::bootstrap {

/// Upper bound on habits entered at first run, chosen so the grid always fits
/// on screen.
inline constexpr std::size_t kMaxHabits = 20;

/// Asks for the habits to track and adds them to `year`.
///
/// Only ever called on a year that has no habits, so there is nothing it could
/// overwrite -- the destructive "create new sheet?" prompt this replaces is gone
/// entirely. Invalid input is re-prompted rather than accepted; end-of-input or
/// an empty answer cancels.
///
/// Returns true if at least one habit was added.
bool populate(Year& year, std::istream& in, std::ostream& out);

}  // namespace ht::bootstrap
