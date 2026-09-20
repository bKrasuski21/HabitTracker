#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Year.hpp"

namespace ht::serialize {

/// Identifies the file and pins the layout below. Bump kFormatVersion on any
/// incompatible change and teach parse() to read both.
inline constexpr std::string_view kMagic = "habittracker";
inline constexpr int kFormatVersion = 2;
inline constexpr char kFieldSeparator = '\t';

/// The text form of a Year:
///
///     habittracker<TAB>2
///     year<TAB>2026
///     habit<TAB><name><TAB><january><TAB>...<TAB><december>
///
/// One line per habit, carrying its own name, so habits are matched between
/// months by name rather than by position. Each month field is one character per
/// day. HabitName forbids tabs and control characters, so no name can ever
/// break the field structure.
[[nodiscard]] std::string toText(const Year& year);

struct ParseError final {
    /// `line` is one-based, or 0 when the problem is not tied to a line.
    ParseError(std::size_t lineNumber, std::string text)
        : line(lineNumber), message(std::move(text)) {}

    std::size_t line;
    std::string message;
};

struct ParseResult final {
    /// Populated if and only if `error` is empty.
    std::optional<Year> year;
    /// Recoverable problems: the data was loaded, but something was repaired.
    std::vector<std::string> warnings;
    /// Set when the file could not be understood at all. Nothing is guessed.
    std::optional<ParseError> error;

    [[nodiscard]] bool ok() const noexcept { return year.has_value(); }
};

/// Parses the text form. Never throws: every failure is reported in the result,
/// because a malformed file is an expected condition, not an exceptional one.
[[nodiscard]] ParseResult parse(std::string_view text);

}  // namespace ht::serialize
