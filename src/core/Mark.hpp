#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ht {

/// The state of one day of one habit.
///
/// The underlying values are part of the on-disk format (see Serialization.hpp)
/// and must not be renumbered without bumping the format version.
enum class Mark : std::uint8_t {
    None = 0,     ///< Not marked.
    Done = 1,     ///< Completed (green).
    Partial = 2,  ///< Partially completed (yellow).
};

inline constexpr std::size_t kMarkCount = 3;

[[nodiscard]] constexpr std::size_t markIndex(Mark mark) noexcept {
    return static_cast<std::size_t>(mark);
}

[[nodiscard]] constexpr char markToChar(Mark mark) noexcept {
    return static_cast<char>('0' + static_cast<int>(mark));
}

[[nodiscard]] constexpr std::optional<Mark> markFromChar(char c) noexcept {
    switch (c) {
        case '0': return Mark::None;
        case '1': return Mark::Done;
        case '2': return Mark::Partial;
        default: return std::nullopt;
    }
}

[[nodiscard]] constexpr std::string_view markName(Mark mark) noexcept {
    switch (mark) {
        case Mark::None: return "none";
        case Mark::Done: return "done";
        case Mark::Partial: return "partial";
    }
    return "none";
}

/// Applying `requested` to a day currently holding `current`.
///
/// Re-applying the mark a day already holds clears it; applying the other mark
/// switches straight to it. `requested` is never Mark::None -- clearing is
/// expressed by asking for the mark that is already there.
[[nodiscard]] constexpr Mark toggledMark(Mark current, Mark requested) noexcept {
    return current == requested ? Mark::None : requested;
}

}  // namespace ht
