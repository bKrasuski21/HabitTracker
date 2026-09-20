#include <stdexcept>
#include <vector>

#include "TestHarness.hpp"
#include "core/Date.hpp"
#include "core/HabitEdits.hpp"
#include "core/HabitName.hpp"
#include "core/HabitTrack.hpp"
#include "core/Mark.hpp"
#include "core/MonthSheet.hpp"
#include "core/Year.hpp"

using namespace ht;

// -- Date -----------------------------------------------------------------

HT_TEST(leapYearsFollowTheGregorianRule) {
    HT_CHECK(isLeapYear(2024));
    HT_CHECK(isLeapYear(2000));   // divisible by 400
    HT_CHECK(!isLeapYear(2100)); // divisible by 100 but not 400
    HT_CHECK(!isLeapYear(2025));
    HT_CHECK(!isLeapYear(2026));
}

HT_TEST(februaryGainsADayInALeapYear) {
    HT_CHECK_EQ(daysInMonth(2025, Month::February), 28U);
    HT_CHECK_EQ(daysInMonth(2024, Month::February), 29U);
    HT_CHECK_EQ(daysInMonth(2100, Month::February), 28U);
    HT_CHECK_EQ(daysInMonth(2000, Month::February), 29U);
}

HT_TEST(everyOtherMonthHasAFixedLength) {
    HT_CHECK_EQ(daysInMonth(2026, Month::January), 31U);
    HT_CHECK_EQ(daysInMonth(2026, Month::April), 30U);
    HT_CHECK_EQ(daysInMonth(2026, Month::December), 31U);
    // A leap year must not disturb anything but February.
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const Month month = monthFromIndex(i);
        if (month == Month::February) {
            continue;
        }
        HT_CHECK_EQ(daysInMonth(2024, month), daysInMonth(2025, month));
    }
}

HT_TEST(anOutOfRangeYearIsRefusedRatherThanGuessed) {
    HT_CHECK_THROWS_AS(daysInMonth(kMinYear - 1, Month::March), std::out_of_range);
    HT_CHECK_THROWS_AS(daysInMonth(kMaxYear + 1, Month::March), std::out_of_range);
    HT_CHECK_THROWS_AS(monthFromIndex(kMonthsPerYear), std::out_of_range);
}

HT_TEST(monthStepWrapsInBothDirections) {
    HT_CHECK(monthStep(Month::December, 1) == Month::January);
    HT_CHECK(monthStep(Month::January, -1) == Month::December);
    HT_CHECK(monthStep(Month::June, 0) == Month::June);
    // Any magnitude of step lands on a real month.
    HT_CHECK(monthStep(Month::January, 25) == Month::February);
    HT_CHECK(monthStep(Month::January, -25) == Month::December);
}

// -- Mark -----------------------------------------------------------------

HT_TEST(reapplyingAMarkClearsItAndTheOtherMarkSwitches) {
    // The old build cleared a yellow square on left click instead of turning it
    // green, so a mark could be removed but never changed.
    HT_CHECK(toggledMark(Mark::None, Mark::Done) == Mark::Done);
    HT_CHECK(toggledMark(Mark::Done, Mark::Done) == Mark::None);
    HT_CHECK(toggledMark(Mark::Partial, Mark::Done) == Mark::Done);
    HT_CHECK(toggledMark(Mark::Done, Mark::Partial) == Mark::Partial);
    HT_CHECK(toggledMark(Mark::Partial, Mark::Partial) == Mark::None);
}

HT_TEST(marksRoundTripThroughTheirCharacter) {
    for (const Mark mark : {Mark::None, Mark::Done, Mark::Partial}) {
        HT_CHECK(markFromChar(markToChar(mark)) == mark);
    }
    HT_CHECK(!markFromChar('x').has_value());
    HT_CHECK(!markFromChar('3').has_value());
}

// -- HabitName ------------------------------------------------------------

HT_TEST(habitNamesRejectAnythingThatWouldCorruptTheFile) {
    HT_CHECK(!HabitName::isValid(""));
    HT_CHECK(!HabitName::isValid("   "));
    HT_CHECK(!HabitName::isValid("has\ttab"));
    HT_CHECK(!HabitName::isValid("has\nnewline"));
    HT_CHECK(!HabitName::isValid(std::string(HabitName::kMaxLength + 1, 'x')));
    HT_CHECK_THROWS_AS(HabitName("bad\tname"), std::invalid_argument);
}

HT_TEST(habitNamesAcceptRealWorldNames) {
    HT_CHECK(HabitName::isValid("gym"));
    // Spaces and commas were both fatal to the old save format.
    HT_CHECK(HabitName::isValid("read for 20 minutes"));
    HT_CHECK(HabitName::isValid("cardio, but gentle"));
    HT_CHECK(HabitName::isValid("练习中文"));
    HT_CHECK_EQ(HabitName("  stretch  ").str(), std::string("stretch"));
}

HT_TEST(habitNameRejectionExplainsItself) {
    HT_CHECK(!HabitName::rejectionReason("").empty());
    HT_CHECK(!HabitName::rejectionReason("a\tb").empty());
    HT_CHECK(HabitName::rejectionReason("gym").empty());
}

// -- HabitTrack -----------------------------------------------------------

HT_TEST(aTrackAlwaysAgreesWithItsOwnLength) {
    const HabitTrack track(HabitName("gym"), 31U);
    HT_CHECK_EQ(track.dayCount(), 31U);
    HT_CHECK_EQ(track.marks().size(), std::size_t{31});
    HT_CHECK(!track.anyMarked());

    HT_CHECK_THROWS_AS(HabitTrack(HabitName("gym"), 0U), std::out_of_range);
    HT_CHECK_THROWS_AS(HabitTrack(HabitName("gym"), 32U), std::out_of_range);
    HT_CHECK_THROWS_AS(HabitTrack(HabitName("gym"), std::vector<Mark>{}),
                       std::out_of_range);
}

HT_TEST(dayAccessIsCheckedInBothDirections) {
    HabitTrack track(HabitName("gym"), 30U);
    HT_CHECK_THROWS_AS(track.mark(30U), std::out_of_range);
    HT_CHECK_THROWS_AS(track.setMark(30U, Mark::Done), std::out_of_range);
    HT_CHECK_THROWS_AS(track.toggle(99U, Mark::Done), std::out_of_range);
    HT_CHECK_NO_THROW(track.setMark(29U, Mark::Done));
    HT_CHECK(track.mark(29U) == Mark::Done);
    HT_CHECK(track.anyMarked());
}

HT_TEST(resizingATrackPreservesTheDaysItKeeps) {
    HabitTrack track(HabitName("gym"), 28U);
    track.setMark(27U, Mark::Partial);

    track.resizeTo(29U);  // common year into a leap year
    HT_CHECK_EQ(track.dayCount(), 29U);
    HT_CHECK(track.mark(27U) == Mark::Partial);
    HT_CHECK(track.mark(28U) == Mark::None);

    track.resizeTo(28U);
    HT_CHECK_EQ(track.dayCount(), 28U);
    HT_CHECK(track.mark(27U) == Mark::Partial);
}

// -- MonthSheet -----------------------------------------------------------

HT_TEST(aSheetRefusesDuplicateHabits) {
    const std::vector<HabitName> names{HabitName("gym"), HabitName("gym")};
    HT_CHECK_THROWS_AS(MonthSheet(2026, Month::January, names), std::invalid_argument);

    MonthSheet sheet(2026, Month::January, std::vector<HabitName>{HabitName("gym")});
    HT_CHECK_THROWS_AS(sheet.addHabit(HabitName("gym")), std::invalid_argument);
}

HT_TEST(aSheetNormalisesTrackLengthsToItsOwnMonth) {
    // A February saved in a common year, loaded into a leap year.
    std::vector<HabitTrack> tracks;
    tracks.emplace_back(HabitName("gym"), 28U);
    const MonthSheet sheet(2024, Month::February, std::move(tracks));
    HT_CHECK_EQ(sheet.dayCount(), 29U);
    HT_CHECK_EQ(sheet.track(0).dayCount(), 29U);
}

HT_TEST(sheetLookupIsByNameAndBoundsChecked) {
    const MonthSheet sheet(2026, Month::March,
                           std::vector<HabitName>{HabitName("gym"), HabitName("read")});
    HT_CHECK(sheet.indexOf(HabitName("read")) == std::optional<std::size_t>{1});
    HT_CHECK(!sheet.indexOf(HabitName("absent")).has_value());
    HT_CHECK_THROWS_AS(sheet.track(2), std::out_of_range);
}

HT_TEST(removingAHabitFromASheetReportsWhetherItWasThere) {
    MonthSheet sheet(2026, Month::March, std::vector<HabitName>{HabitName("gym")});
    HT_CHECK(!sheet.removeHabit(HabitName("absent")));
    HT_CHECK(sheet.removeHabit(HabitName("gym")));
    HT_CHECK_EQ(sheet.habitCount(), std::size_t{0});
}

// -- Year -----------------------------------------------------------------

HT_TEST(aYearGivesEveryMonthTheSameHabitsInTheSameOrder) {
    const Year year(2026, {HabitName("gym"), HabitName("read")});
    HT_CHECK_EQ(year.habitCount(), std::size_t{2});
    for (std::size_t i = 0; i < kMonthsPerYear; ++i) {
        const MonthSheet& sheet = year.month(monthFromIndex(i));
        HT_CHECK(sheet.month() == monthFromIndex(i));
        HT_CHECK_EQ(sheet.year(), 2026);
        HT_CHECK(sheet.habitNames() == year.habitNames());
        HT_CHECK_EQ(sheet.dayCount(), daysInMonth(2026, monthFromIndex(i)));
    }
}

HT_TEST(addingOrRemovingAHabitTouchesAllTwelveMonths) {
    Year year(2026, {HabitName("gym")});
    year.addHabit(HabitName("read"));
    for (const MonthSheet& sheet : year.months()) {
        HT_CHECK_EQ(sheet.habitCount(), std::size_t{2});
        HT_CHECK(sheet.indexOf(HabitName("read")).has_value());
    }

    HT_CHECK(year.removeHabit(HabitName("gym")));
    for (const MonthSheet& sheet : year.months()) {
        HT_CHECK_EQ(sheet.habitCount(), std::size_t{1});
        HT_CHECK(!sheet.indexOf(HabitName("gym")).has_value());
    }
    HT_CHECK(!year.removeHabit(HabitName("gym")));
    HT_CHECK_THROWS_AS(year.addHabit(HabitName("read")), std::invalid_argument);
}

HT_TEST(aYearRefusesMisalignedSheets) {
    // The old format matched habits between months by line position, so this is
    // exactly the state that used to scramble a year of data.
    auto build = [](std::size_t index) {
        const Month month = monthFromIndex(index);
        std::vector<HabitName> names{HabitName("gym"), HabitName("read")};
        if (month == Month::July) {
            names = {HabitName("read"), HabitName("gym")};  // reordered
        }
        return MonthSheet(2026, month, names);
    };
    std::array<MonthSheet, kMonthsPerYear> months{
        build(0), build(1), build(2),  build(3),  build(4),  build(5),
        build(6), build(7), build(8),  build(9),  build(10), build(11)};
    HT_CHECK_THROWS_AS(Year(2026, std::move(months)), std::invalid_argument);
}

HT_TEST(aYearRefusesSheetsInTheWrongOrder) {
    auto sheet = [](Month month) {
        return MonthSheet(2026, month, std::vector<HabitName>{HabitName("gym")});
    };
    std::array<MonthSheet, kMonthsPerYear> months{
        sheet(Month::February), sheet(Month::January),   sheet(Month::March),
        sheet(Month::April),    sheet(Month::May),       sheet(Month::June),
        sheet(Month::July),     sheet(Month::August),    sheet(Month::September),
        sheet(Month::October),  sheet(Month::November),  sheet(Month::December)};
    HT_CHECK_THROWS_AS(Year(2026, std::move(months)), std::invalid_argument);
}

HT_TEST(togglingThroughTheYearIsCheckedAndReturnsTheResult) {
    Year year(2026, {HabitName("gym")});
    HT_CHECK(year.toggle(Month::March, 0, 4U, Mark::Done) == Mark::Done);
    HT_CHECK(year.month(Month::March).track(0).mark(4U) == Mark::Done);
    HT_CHECK(year.toggle(Month::March, 0, 4U, Mark::Partial) == Mark::Partial);
    HT_CHECK(year.toggle(Month::March, 0, 4U, Mark::Partial) == Mark::None);
    HT_CHECK(year.anyMarked() == false);

    HT_CHECK_THROWS_AS(year.toggle(Month::March, 1, 0U, Mark::Done), std::out_of_range);
    HT_CHECK_THROWS_AS(year.toggle(Month::February, 0, 28U, Mark::Done),
                       std::out_of_range);  // 2026 February has 28 days
    HT_CHECK_THROWS_AS(Year(kMinYear - 1), std::out_of_range);
}

// -- HabitEdits -----------------------------------------------------------

namespace {

[[nodiscard]] Year yearWith(const std::vector<HabitName>& names) {
    return Year(2026, names);
}

// Not named add/remove: an unqualified removeEdit("gym") resolves to
// std::remove(const char*) -- the one that deletes a file -- in preference to an
// overload taking std::string_view.
[[nodiscard]] HabitEdit addEdit(std::string_view name) {
    return HabitEdit{HabitEdit::Kind::Add, HabitName(name)};
}

[[nodiscard]] HabitEdit removeEdit(std::string_view name) {
    return HabitEdit{HabitEdit::Kind::Remove, HabitName(name)};
}

}  // namespace

HT_TEST(aBatchOfEditsAppliesInOrderAndKeepsEveryMonthAligned) {
    Year year = yearWith({HabitName("gym"), HabitName("read")});
    const HabitEditReport report =
        applyHabitEdits(year, {removeEdit("gym"), addEdit("stretch"), addEdit("floss")});

    HT_CHECK(report.ok());
    HT_CHECK_EQ(report.applied.size(), std::size_t{3});
    HT_CHECK_EQ(year.habitCount(), std::size_t{3});
    HT_CHECK(!year.indexOf(HabitName("gym")).has_value());
    HT_CHECK(year.indexOf(HabitName("stretch")).has_value());

    // The Year invariant that matters: a habit exists in all twelve months.
    for (const MonthSheet& sheet : year.months()) {
        HT_CHECK_EQ(sheet.habitCount(), std::size_t{3});
        HT_CHECK(sheet.indexOf(HabitName("floss")).has_value());
    }
}

HT_TEST(oneBadEditLeavesTheWholeYearUntouched) {
    Year year = yearWith({HabitName("gym")});
    const Year before = year;

    const HabitEditReport report =
        applyHabitEdits(year, {addEdit("read"), removeEdit("nonexistent"), addEdit("floss")});

    HT_CHECK(!report.ok());
    HT_CHECK_EQ(report.problems.size(), std::size_t{1});
    HT_CHECK(report.applied.empty());
    // The two valid edits either side of the bad one must not have landed.
    HT_CHECK(year == before);
}

HT_TEST(everyProblemIsReportedNotJustTheFirst) {
    Year year = yearWith({HabitName("gym")});
    const HabitEditReport report =
        applyHabitEdits(year, {addEdit("gym"), removeEdit("missing"), addEdit("gym")});
    HT_CHECK_EQ(report.problems.size(), std::size_t{3});
}

HT_TEST(namingTheSameHabitTwiceInOneBatchIsRefused) {
    Year year = yearWith({HabitName("gym")});
    HT_CHECK(!applyHabitEdits(year, {addEdit("read"), addEdit("read")}).ok());
    HT_CHECK(!applyHabitEdits(year, {removeEdit("gym"), removeEdit("gym")}).ok());
    HT_CHECK_EQ(year.habitCount(), std::size_t{1});
}

HT_TEST(aHabitCanBeRemovedAndReaddedInOneBatchStartingFresh) {
    Year year = yearWith({HabitName("gym")});
    year.toggle(Month::March, 0, 4U, Mark::Done);

    HT_CHECK(applyHabitEdits(year, {removeEdit("gym"), addEdit("gym")}).ok());
    HT_CHECK_EQ(year.habitCount(), std::size_t{1});
    HT_CHECK(year.anyMarked() == false);  // Re-adding starts unmarked.
}

HT_TEST(removalReportsTheMarksItWouldDiscard) {
    Year year = yearWith({HabitName("gym"), HabitName("read")});
    year.toggle(Month::January, 0, 0U, Mark::Done);
    year.toggle(Month::March, 0, 4U, Mark::Partial);
    year.toggle(Month::March, 1, 4U, Mark::Done);

    HT_CHECK_EQ(markedDayCount(year, HabitName("gym")), std::size_t{2});
    HT_CHECK_EQ(markedDayCount(year, HabitName("read")), std::size_t{1});
    HT_CHECK_EQ(markedDayCount(year, HabitName("never tracked")), std::size_t{0});

    const HabitEditReport plan = planHabitEdits(year, {removeEdit("gym")});
    HT_CHECK(plan.ok());
    HT_CHECK_EQ(plan.marksDiscarded, std::size_t{2});
}

HT_TEST(planningNeverChangesTheYear) {
    Year year = yearWith({HabitName("gym")});
    const Year before = year;
    (void)planHabitEdits(year, {addEdit("read"), removeEdit("gym")});
    HT_CHECK(year == before);
}

HT_TEST(anEmptyBatchIsAcceptedAndDoesNothing) {
    Year year = yearWith({HabitName("gym")});
    const HabitEditReport report = applyHabitEdits(year, std::vector<HabitEdit>{});
    HT_CHECK(report.ok());
    HT_CHECK(report.applied.empty());
    HT_CHECK_EQ(report.marksDiscarded, std::size_t{0});
    HT_CHECK_EQ(year.habitCount(), std::size_t{1});
}

HT_TEST(removingTheLastHabitLeavesAnEmptyButValidYear) {
    Year year = yearWith({HabitName("gym")});
    HT_CHECK(applyHabitEdits(year, {removeEdit("gym")}).ok());
    HT_CHECK(year.empty());
    for (const MonthSheet& sheet : year.months()) {
        HT_CHECK_EQ(sheet.habitCount(), std::size_t{0});
    }
}
