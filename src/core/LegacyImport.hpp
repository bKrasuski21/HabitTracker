#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Year.hpp"

namespace ht::legacy {

/// The pre-2.0 layout: twelve files named month0..month11 in the data
/// directory, one "name,marks" line per habit, no version and no year recorded.
inline constexpr std::size_t kLegacyFileCount = kMonthsPerYear;

[[nodiscard]] std::filesystem::path legacyFilePath(
    const std::filesystem::path& dataDirectory, std::size_t monthIndex);

/// True if at least one month file of the old layout is present.
[[nodiscard]] bool hasLegacyData(const std::filesystem::path& dataDirectory);

struct ImportResult final {
    std::optional<Year> year;
    std::vector<std::string> warnings;
    std::optional<std::string> error;

    [[nodiscard]] bool ok() const noexcept { return year.has_value(); }
};

/// Reads the old layout into a Year numbered `yearNumber`.
///
/// The old format recorded no year, so the caller must supply one. Habits are
/// taken from the union of all twelve files rather than from month0 alone, and
/// are matched between months by name: the old position-based matching is
/// exactly the bug this import exists to stop propagating. A habit missing from
/// some month is imported as unmarked there, with a warning.
[[nodiscard]] ImportResult importYear(const std::filesystem::path& dataDirectory,
                                      int yearNumber);

/// Parses one legacy line into a name and its mark characters. Splits on the
/// LAST comma, so a name containing a comma round-trips instead of truncating.
[[nodiscard]] std::optional<std::pair<std::string_view, std::string_view>>
splitLegacyLine(std::string_view line);

}  // namespace ht::legacy
