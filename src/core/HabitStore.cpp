#include "core/HabitStore.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>

#include "core/LegacyImport.hpp"
#include "core/Serialization.hpp"

namespace ht {
namespace {

[[nodiscard]] std::optional<std::string> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        return std::nullopt;
    }
    return buffer.str();
}

}  // namespace

std::filesystem::path HabitStore::filePathFor(const std::filesystem::path& dataDirectory,
                                              int yearNumber) {
    return dataDirectory / (std::to_string(yearNumber) + ".habits");
}

HabitStore::HabitStore(std::filesystem::path dataDirectory, int yearNumber)
    : dataDirectory_(std::move(dataDirectory)),
      filePath_(filePathFor(dataDirectory_, yearNumber)),
      year_(loadOrCreate(yearNumber)) {}

Year HabitStore::loadOrCreate(int yearNumber) {
    if (!isYearInRange(yearNumber)) {
        throw std::out_of_range("year " + std::to_string(yearNumber) +
                                " is out of range");
    }

    std::error_code ec;
    if (std::filesystem::exists(filePath_, ec)) {
        const std::optional<std::string> contents = readFile(filePath_);
        if (!contents.has_value()) {
            throw LoadError(filePath_, "the file could not be read");
        }
        serialize::ParseResult parsed = serialize::parse(*contents);
        if (!parsed.ok()) {
            const serialize::ParseError& error = *parsed.error;
            const std::string where =
                error.line == 0 ? std::string{} : " (line " + std::to_string(error.line) + ")";
            throw LoadError(filePath_, error.message + where);
        }
        loadMessages_ = std::move(parsed.warnings);
        if (parsed.year->number() != yearNumber) {
            // The file names the year it holds; trust the file over the request.
            loadMessages_.push_back("the file holds " +
                                    std::to_string(parsed.year->number()) +
                                    ", not " + std::to_string(yearNumber));
        }
        return std::move(*parsed.year);
    }

    if (legacy::hasLegacyData(dataDirectory_)) {
        legacy::ImportResult imported = legacy::importYear(dataDirectory_, yearNumber);
        if (!imported.ok()) {
            throw LoadError(dataDirectory_,
                            "old month files are present but could not be imported: " +
                                *imported.error);
        }
        loadMessages_ = std::move(imported.warnings);
        importedLegacy_ = true;
        dirty_ = true;  // Nothing on disk in the new format yet.
        return std::move(*imported.year);
    }

    return Year(yearNumber);
}

bool HabitStore::writeAtomically(const std::string& contents) {
    std::error_code ec;
    std::filesystem::create_directories(dataDirectory_, ec);
    if (ec) {
        lastError_ = "could not create " + dataDirectory_.string() + ": " + ec.message();
        return false;
    }

    // Keep one previous generation the first time this run replaces the file.
    if (!backedUp_ && std::filesystem::exists(filePath_, ec)) {
        std::filesystem::path backup = filePath_;
        backup += ".bak";
        std::filesystem::copy_file(filePath_, backup,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        backedUp_ = true;  // One attempt per run; a failed backup must not block saving.
        ec.clear();
    }

    std::filesystem::path temporary = filePath_;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            lastError_ = "could not open " + temporary.string() + " for writing";
            return false;
        }
        file << contents;
        file.flush();
        if (!file) {
            lastError_ = "could not write to " + temporary.string();
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }

    std::filesystem::rename(temporary, filePath_, ec);
    if (ec) {
        lastError_ = "could not replace " + filePath_.string() + ": " + ec.message();
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    lastError_.clear();
    return true;
}

bool HabitStore::save() {
    if (!writeAtomically(serialize::toText(year_))) {
        return false;
    }
    dirty_ = false;
    return true;
}

bool HabitStore::saveIfDirty() { return dirty_ ? save() : true; }

HabitStore::~HabitStore() {
    if (!dirty_) {
        return;
    }
    // A destructor must not throw, and losing the user's marks silently is worse
    // than a message on stderr.
    try {
        if (!save()) {
            std::cerr << "HabitTracker: could not save to " << filePath_.string() << ": "
                      << lastError_ << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "HabitTracker: could not save to " << filePath_.string() << ": "
                  << e.what() << '\n';
    } catch (...) {
        std::cerr << "HabitTracker: could not save to " << filePath_.string() << '\n';
    }
}

}  // namespace ht
