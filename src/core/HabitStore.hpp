#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/Year.hpp"

namespace ht {

/// Thrown when an existing data file is present but cannot be understood.
///
/// This is deliberately fatal rather than recoverable: starting from an empty
/// year and later saving over a file we failed to read is precisely how the
/// previous version destroyed data. The file is never touched when this throws.
class LoadError final : public std::runtime_error {
public:
    LoadError(std::filesystem::path path, const std::string& message)
        : std::runtime_error(message), path_(std::move(path)) {}

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

/// Owns a Year and its file on disk.
///
/// Persistence is tied to the object's lifetime rather than to any control-flow
/// path: the constructor loads, the destructor flushes. Saving is atomic (a
/// temporary file is renamed over the target), and the previous contents are
/// kept alongside as a .bak, so an interrupted or failed write can never leave a
/// half-written year behind.
class HabitStore final {
public:
    /// Loads `<year>.habits` from `dataDirectory`, importing the pre-2.0 layout
    /// if that file is absent and old month files are present. Starts empty when
    /// there is nothing to load.
    ///
    /// Throws LoadError if a data file exists but cannot be parsed, and
    /// std::out_of_range if `yearNumber` is not representable.
    HabitStore(std::filesystem::path dataDirectory, int yearNumber);

    /// Flushes if dirty. Reports failure on stderr; never throws.
    ~HabitStore();

    HabitStore(const HabitStore&) = delete;
    HabitStore& operator=(const HabitStore&) = delete;
    HabitStore(HabitStore&&) = delete;
    HabitStore& operator=(HabitStore&&) = delete;

    [[nodiscard]] const Year& year() const noexcept { return year_; }

    /// Mutable access, conservatively marking the store dirty. Callers that
    /// merely read should use year().
    [[nodiscard]] Year& mutableYear() noexcept {
        dirty_ = true;
        return year_;
    }

    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    [[nodiscard]] const std::filesystem::path& filePath() const noexcept {
        return filePath_;
    }
    /// Recoverable problems encountered while loading, in the order found.
    [[nodiscard]] const std::vector<std::string>& loadMessages() const noexcept {
        return loadMessages_;
    }
    /// True when the year was recovered from the pre-2.0 layout on this run.
    [[nodiscard]] bool importedLegacyData() const noexcept { return importedLegacy_; }

    /// Writes the year to disk atomically. Returns true on success; on failure
    /// the existing file is left untouched and the store stays dirty.
    bool save();

    /// save() if anything has changed, otherwise a no-op returning true.
    bool saveIfDirty();

    /// The most recent save failure, or an empty string if the last save worked.
    [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }

    [[nodiscard]] static std::filesystem::path filePathFor(
        const std::filesystem::path& dataDirectory, int yearNumber);

private:
    Year loadOrCreate(int yearNumber);
    bool writeAtomically(const std::string& contents);

    // Declaration order is load-bearing: members are initialized in this order,
    // and year_ is built by loadOrCreate(), which reports back into the members
    // above it. Anything it writes to must be declared before year_ or its own
    // default initializer would run afterwards and overwrite the result.
    std::filesystem::path dataDirectory_;
    std::filesystem::path filePath_;
    std::vector<std::string> loadMessages_;
    std::string lastError_;
    bool dirty_ = false;
    bool importedLegacy_ = false;
    bool backedUp_ = false;
    Year year_;
};

}  // namespace ht
