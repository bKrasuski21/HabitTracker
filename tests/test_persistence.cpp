#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "TestHarness.hpp"
#include "core/HabitStore.hpp"
#include "core/LegacyImport.hpp"
#include "core/Serialization.hpp"

using namespace ht;

namespace {

/// A temporary directory removed when the test finishes, whatever happens.
class ScopedDirectory final {
public:
    ScopedDirectory() {
        static std::mt19937_64 rng{std::random_device{}()};
        path_ = std::filesystem::temp_directory_path() /
                ("habittracker-test-" + std::to_string(rng()));
        std::filesystem::create_directories(path_);
    }
    ~ScopedDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    ScopedDirectory(const ScopedDirectory&) = delete;
    ScopedDirectory& operator=(const ScopedDirectory&) = delete;
    ScopedDirectory(ScopedDirectory&&) = delete;
    ScopedDirectory& operator=(ScopedDirectory&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

void writeFile(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << contents;
}

[[nodiscard]] std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] Year sampleYear() {
    Year year(2026, {HabitName("gym"), HabitName("read for 20 minutes")});
    year.toggle(Month::January, 0, 0U, Mark::Done);
    year.toggle(Month::February, 1, 27U, Mark::Partial);
    year.toggle(Month::December, 0, 30U, Mark::Done);
    return year;
}

}  // namespace

// -- Serialization --------------------------------------------------------

HT_TEST(aYearSurvivesARoundTripThroughText) {
    const Year original = sampleYear();
    const serialize::ParseResult parsed = serialize::parse(serialize::toText(original));
    HT_CHECK(parsed.ok());
    HT_CHECK(parsed.warnings.empty());
    if (parsed.ok()) {
        HT_CHECK(*parsed.year == original);
    }
}

HT_TEST(namesWithSpacesAndCommasRoundTripIntact) {
    // "cardio, but gentle" split into a wrong name and junk data in the old
    // comma-delimited format.
    Year year(2026, {HabitName("cardio, but gentle"), HabitName("read for 20 minutes")});
    year.toggle(Month::June, 0, 3U, Mark::Done);
    const serialize::ParseResult parsed = serialize::parse(serialize::toText(year));
    HT_CHECK(parsed.ok());
    if (parsed.ok()) {
        HT_CHECK(*parsed.year == year);
        const std::vector<HabitName> names = parsed.year->habitNames();
        HT_CHECK_EQ(names.front().str(), std::string("cardio, but gentle"));
    }
}

HT_TEST(anEmptyYearRoundTrips) {
    const Year empty(2026);
    const serialize::ParseResult parsed = serialize::parse(serialize::toText(empty));
    HT_CHECK(parsed.ok());
    if (parsed.ok()) {
        HT_CHECK(parsed.year->empty());
        HT_CHECK_EQ(parsed.year->number(), 2026);
    }
}

HT_TEST(habitsAreMatchedByNameNotByPosition) {
    // Reordering the habit lines must not move anyone's marks.
    Year year(2026, {HabitName("gym"), HabitName("read")});
    year.toggle(Month::May, 1, 2U, Mark::Done);  // "read" on 3 May

    const std::string text = serialize::toText(year);
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    HT_CHECK_EQ(lines.size(), std::size_t{4});
    std::swap(lines[2], lines[3]);

    std::string reordered;
    for (const std::string& each : lines) {
        reordered += each + "\n";
    }

    const serialize::ParseResult parsed = serialize::parse(reordered);
    HT_CHECK(parsed.ok());
    if (parsed.ok()) {
        const std::optional<std::size_t> read = parsed.year->indexOf(HabitName("read"));
        HT_CHECK(read.has_value());
        if (read.has_value()) {
            HT_CHECK(parsed.year->month(Month::May).track(*read).mark(2U) == Mark::Done);
            HT_CHECK(parsed.year->month(Month::May).track(*read).mark(3U) != Mark::Done);
        }
    }
}

HT_TEST(structuralDamageIsReportedRatherThanGuessedAt) {
    HT_CHECK(!serialize::parse("").ok());
    HT_CHECK(!serialize::parse("not a habit file\n").ok());
    HT_CHECK(!serialize::parse("habittracker\t999\nyear\t2026\n").ok());
    HT_CHECK(!serialize::parse("habittracker\t2\n").ok());
    HT_CHECK(!serialize::parse("habittracker\t2\nyear\tnineteen\n").ok());
    HT_CHECK(!serialize::parse("habittracker\t2\nyear\t1\n").ok());
    // A habit line with too few month fields.
    HT_CHECK(!serialize::parse("habittracker\t2\nyear\t2026\nhabit\tgym\t000\n").ok());
    // The same habit twice.
    const Year year(2026, {HabitName("gym")});
    std::string text = serialize::toText(year);
    std::istringstream stream(text);
    std::string header;
    std::string yearLine;
    std::string habitLine;
    std::getline(stream, header);
    std::getline(stream, yearLine);
    std::getline(stream, habitLine);
    HT_CHECK(!serialize::parse(header + "\n" + yearLine + "\n" + habitLine + "\n" +
                               habitLine + "\n")
                  .ok());
}

HT_TEST(blankLinesAndCommentsAreIgnored) {
    const Year year(2026, {HabitName("gym")});
    const std::string text = "# saved by hand\n\n" + serialize::toText(year) + "\n";
    const serialize::ParseResult parsed = serialize::parse(text);
    HT_CHECK(parsed.ok());
    if (parsed.ok()) {
        HT_CHECK(*parsed.year == year);
    }
}

HT_TEST(windowsLineEndingsAreAccepted) {
    const Year year(2026, {HabitName("gym")});
    std::string text = serialize::toText(year);
    std::string withCrLf;
    for (const char c : text) {
        if (c == '\n') {
            withCrLf += '\r';
        }
        withCrLf += c;
    }
    const serialize::ParseResult parsed = serialize::parse(withCrLf);
    HT_CHECK(parsed.ok());
    if (parsed.ok()) {
        HT_CHECK(*parsed.year == year);
    }
}

HT_TEST(damagedDaysAreRepairedWithAWarningRatherThanLosingTheYear) {
    // A February field one day short, and an unreadable character in March.
    std::string text = "habittracker\t2\nyear\t2024\nhabit\tgym";
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const Month month = monthFromIndex(i);
        text += '\t';
        if (month == Month::February) {
            text += std::string(28, '0');  // 2024 February has 29 days
        } else if (month == Month::March) {
            text += "1x" + std::string(29, '0');
        } else {
            text += std::string(daysInMonth(2024, month), '0');
        }
    }
    text += '\n';

    const serialize::ParseResult parsed = serialize::parse(text);
    HT_CHECK(parsed.ok());
    HT_CHECK(parsed.warnings.size() >= 2);
    if (parsed.ok()) {
        HT_CHECK_EQ(parsed.year->month(Month::February).track(0).dayCount(), 29U);
        HT_CHECK(parsed.year->month(Month::February).track(0).mark(28U) == Mark::None);
        HT_CHECK(parsed.year->month(Month::March).track(0).mark(0U) == Mark::Done);
        HT_CHECK(parsed.year->month(Month::March).track(0).mark(1U) == Mark::None);
    }
}

// -- Legacy import --------------------------------------------------------

HT_TEST(legacyLinesSplitOnTheLastCommaSoNamesSurvive) {
    const auto plain = legacy::splitLegacyLine("gym,0101");
    HT_CHECK(plain.has_value());
    if (plain.has_value()) {
        HT_CHECK_EQ(std::string(plain->first), std::string("gym"));
        HT_CHECK_EQ(std::string(plain->second), std::string("0101"));
    }
    const auto commaName = legacy::splitLegacyLine("cardio, but gentle,0101");
    HT_CHECK(commaName.has_value());
    if (commaName.has_value()) {
        HT_CHECK_EQ(std::string(commaName->first), std::string("cardio, but gentle"));
        HT_CHECK_EQ(std::string(commaName->second), std::string("0101"));
    }
    HT_CHECK(!legacy::splitLegacyLine("no comma here").has_value());
}

HT_TEST(theOldMonthFilesImportAndKeepEveryMarkOnTheRightHabit) {
    const ScopedDirectory dir;
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const unsigned days = daysInMonth(2026, monthFromIndex(i));
        std::string gym(days, '0');
        std::string read(days, '0');
        if (i == 0) {
            gym[0] = '1';
            read[4] = '2';
        }
        // October lists the habits in the opposite order: the old importer
        // matched by position and would have swapped their data.
        const std::string contents = (i == 9)
                                         ? "read," + read + "\ngym," + gym + "\n"
                                         : "gym," + gym + "\nread," + read + "\n";
        writeFile(legacy::legacyFilePath(dir.path(), i), contents);
    }

    HT_CHECK(legacy::hasLegacyData(dir.path()));
    const legacy::ImportResult imported = legacy::importYear(dir.path(), 2026);
    HT_CHECK(imported.ok());
    if (!imported.ok()) {
        return;
    }
    const Year& year = *imported.year;
    HT_CHECK_EQ(year.habitCount(), std::size_t{2});

    const std::optional<std::size_t> gym = year.indexOf(HabitName("gym"));
    const std::optional<std::size_t> read = year.indexOf(HabitName("read"));
    HT_CHECK(gym.has_value() && read.has_value());
    if (gym.has_value() && read.has_value()) {
        HT_CHECK(year.month(Month::January).track(*gym).mark(0U) == Mark::Done);
        HT_CHECK(year.month(Month::January).track(*read).mark(4U) == Mark::Partial);
        HT_CHECK(year.month(Month::October).track(*gym).mark(0U) == Mark::None);
    }
}

HT_TEST(aShortLegacyFebruaryImportsIntoALeapYear) {
    const ScopedDirectory dir;
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        writeFile(legacy::legacyFilePath(dir.path(), i),
                  "gym," + std::string(28, '0') + "\n");
    }
    const legacy::ImportResult imported = legacy::importYear(dir.path(), 2024);
    HT_CHECK(imported.ok());
    if (imported.ok()) {
        HT_CHECK_EQ(imported.year->month(Month::February).track(0).dayCount(), 29U);
        HT_CHECK_EQ(imported.year->month(Month::January).track(0).dayCount(), 31U);
    }
}

HT_TEST(anEmptyLegacySetIsReportedRatherThanImportedAsNothing) {
    const ScopedDirectory dir;
    writeFile(legacy::legacyFilePath(dir.path(), 0), "\n\n");
    const legacy::ImportResult imported = legacy::importYear(dir.path(), 2026);
    HT_CHECK(!imported.ok());
    HT_CHECK(imported.error.has_value());
}

// -- HabitStore -----------------------------------------------------------

HT_TEST(aStoreStartsEmptyAndWritesWhatItIsGiven) {
    const ScopedDirectory dir;
    {
        HabitStore store(dir.path(), 2026);
        HT_CHECK(store.year().empty());
        HT_CHECK(!store.dirty());
        store.mutableYear().addHabit(HabitName("gym"));
        HT_CHECK(store.dirty());
        HT_CHECK(store.save());
        HT_CHECK(!store.dirty());
    }
    HT_CHECK(std::filesystem::exists(HabitStore::filePathFor(dir.path(), 2026)));

    HabitStore reopened(dir.path(), 2026);
    HT_CHECK_EQ(reopened.year().habitCount(), std::size_t{1});
}

HT_TEST(theDestructorFlushesUnsavedChanges) {
    // The old build saved only when one particular event arrived, so anything
    // else -- a crash, a kill -- lost the session.
    const ScopedDirectory dir;
    {
        HabitStore store(dir.path(), 2026);
        store.mutableYear().addHabit(HabitName("gym"));
        store.mutableYear().toggle(Month::April, 0, 9U, Mark::Done);
        // No explicit save.
    }
    HabitStore reopened(dir.path(), 2026);
    HT_CHECK_EQ(reopened.year().habitCount(), std::size_t{1});
    HT_CHECK(reopened.year().month(Month::April).track(0).mark(9U) == Mark::Done);
}

HT_TEST(anUnreadableFileIsNeverOverwritten) {
    const ScopedDirectory dir;
    const std::filesystem::path path = HabitStore::filePathFor(dir.path(), 2026);
    const std::string original = "this is not a habit file at all\n";
    writeFile(path, original);

    bool threw = false;
    try {
        HabitStore store(dir.path(), 2026);
    } catch (const LoadError&) {
        threw = true;
    }
    HT_CHECK(threw);
    HT_CHECK_EQ(readFile(path), original);
}

HT_TEST(savingKeepsOnePreviousGenerationAndLeavesNoTemporary) {
    const ScopedDirectory dir;
    const std::filesystem::path path = HabitStore::filePathFor(dir.path(), 2026);
    {
        HabitStore store(dir.path(), 2026);
        store.mutableYear().addHabit(HabitName("gym"));
        HT_CHECK(store.save());
    }
    const std::string first = readFile(path);
    {
        HabitStore store(dir.path(), 2026);
        store.mutableYear().addHabit(HabitName("read"));
        HT_CHECK(store.save());
    }

    std::filesystem::path backup = path;
    backup += ".bak";
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    HT_CHECK(std::filesystem::exists(backup));
    HT_CHECK(!std::filesystem::exists(temporary));
    HT_CHECK_EQ(readFile(backup), first);
    HT_CHECK(readFile(path) != first);
}

HT_TEST(aStoreImportsTheOldLayoutOnceAndThenUsesTheNewFile) {
    const ScopedDirectory dir;
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const unsigned days = daysInMonth(2026, monthFromIndex(i));
        std::string marks(days, '0');
        marks[0] = '1';
        writeFile(legacy::legacyFilePath(dir.path(), i), "gym," + marks + "\n");
    }

    {
        HabitStore store(dir.path(), 2026);
        HT_CHECK(store.importedLegacyData());
        HT_CHECK(store.dirty());  // nothing in the new format on disk yet
        HT_CHECK_EQ(store.year().habitCount(), std::size_t{1});
        HT_CHECK(store.year().month(Month::January).track(0).mark(0U) == Mark::Done);
        HT_CHECK(store.save());
    }

    // The old files are left alone, and are not read a second time.
    HT_CHECK(std::filesystem::exists(legacy::legacyFilePath(dir.path(), 0)));
    HabitStore reopened(dir.path(), 2026);
    HT_CHECK(!reopened.importedLegacyData());
    HT_CHECK(!reopened.dirty());
    HT_CHECK(reopened.year().month(Month::January).track(0).mark(0U) == Mark::Done);
}

HT_TEST(theStoreCreatesItsDataDirectory) {
    const ScopedDirectory dir;
    const std::filesystem::path nested = dir.path() / "a" / "b";
    HabitStore store(nested, 2026);
    store.mutableYear().addHabit(HabitName("gym"));
    HT_CHECK(store.save());
    HT_CHECK(std::filesystem::exists(HabitStore::filePathFor(nested, 2026)));
}

HT_TEST(anOutOfRangeYearIsRefusedByTheStore) {
    const ScopedDirectory dir;
    HT_CHECK_THROWS_AS(HabitStore(dir.path(), kMinYear - 1), std::out_of_range);
}
