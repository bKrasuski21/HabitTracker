#pragma once

#include <filesystem>

namespace ht::paths {

/// Directory containing the running executable, or an empty path if the
/// platform will not say. Everything the app ships with is resolved against this
/// rather than the working directory, so the app behaves the same whether it is
/// launched from its own folder, a shortcut, or a PATH entry.
[[nodiscard]] std::filesystem::path executableDirectory();

/// First existing candidate, searching the executable directory and then the
/// working directory. Returns an empty path if `fileName` is in neither.
[[nodiscard]] std::filesystem::path locateResource(const std::filesystem::path& fileName);

/// Where habit data lives by default: $HABITTRACKER_DATA_DIR if set; otherwise
/// whichever of "data" beside the executable or "data" in the working directory
/// already exists, preferring the former. Falls back to beside the executable,
/// which need not exist yet -- the store creates it.
[[nodiscard]] std::filesystem::path defaultDataDirectory();

}  // namespace ht::paths
