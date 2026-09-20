#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

/// A minimal test harness.
///
/// Deliberately dependency-free: the whole suite builds and runs from a clean
/// checkout with no network access, which keeps `make test` and CI honest. Swap
/// in Catch2 or doctest if the suite ever outgrows it.
namespace testing {

using TestFunction = void (*)();

struct TestCase final {
    const char* name;
    TestFunction function;
};

[[nodiscard]] std::vector<TestCase>& registry();

/// Failures recorded by the currently running test.
[[nodiscard]] int& currentFailures();

void reportFailure(const char* file, int line, const std::string& message);

/// Runs every registered test. Returns the number of failing tests.
[[nodiscard]] int runAll();

struct Registrar final {
    Registrar(const char* name, TestFunction function) {
        registry().push_back(TestCase{name, function});
    }
};

/// Renders a value if it is streamable, and a placeholder if it is not, so the
/// macros work for every type without requiring operator<<.
template <typename T>
[[nodiscard]] std::string describe(const T& value) {
    if constexpr (requires(std::ostream& os) { os << value; }) {
        std::ostringstream out;
        out << value;
        return out.str();
    } else {
        return "<not printable>";
    }
}

}  // namespace testing

#define HT_TEST(name)                                                    \
    static void name();                                                  \
    static const ::testing::Registrar ht_registrar_##name(#name, &name); \
    static void name()

#define HT_CHECK(expression)                                                       \
    do {                                                                           \
        if (!(expression)) {                                                       \
            ::testing::reportFailure(__FILE__, __LINE__, "expected " #expression); \
        }                                                                          \
    } while (false)

/// Both sides are bound BY VALUE, not by const reference: an expression such as
/// `year.habitNames().front().str()` yields a reference into a temporary, and
/// binding a reference to it does not extend that temporary's life. Copying is
/// cheap at test sizes and removes the whole hazard.
#define HT_CHECK_EQ(actual, expected)                                             \
    do {                                                                          \
        const auto ht_actual = (actual);                                          \
        const auto ht_expected = (expected);                                      \
        if (!(ht_actual == ht_expected)) {                                        \
            ::testing::reportFailure(__FILE__, __LINE__,                          \
                                     std::string(#actual) + " == " + #expected +  \
                                         "\n      actual:   " +                   \
                                         ::testing::describe(ht_actual) +         \
                                         "\n      expected: " +                   \
                                         ::testing::describe(ht_expected));       \
        }                                                                         \
    } while (false)

#define HT_CHECK_THROWS_AS(expression, exceptionType)                             \
    do {                                                                          \
        bool ht_threw = false;                                                    \
        try {                                                                     \
            (void)(expression);                                                   \
        } catch (const exceptionType&) {                                          \
            ht_threw = true;                                                      \
        } catch (...) {                                                           \
            ::testing::reportFailure(__FILE__, __LINE__,                          \
                                     #expression " threw the wrong exception "    \
                                     "type (wanted " #exceptionType ")");         \
            ht_threw = true;                                                      \
        }                                                                         \
        if (!ht_threw) {                                                          \
            ::testing::reportFailure(__FILE__, __LINE__,                          \
                                     #expression " did not throw " #exceptionType); \
        }                                                                         \
    } while (false)

#define HT_CHECK_NO_THROW(expression)                                           \
    do {                                                                        \
        try {                                                                   \
            (void)(expression);                                                 \
        } catch (const std::exception& ht_error) {                              \
            ::testing::reportFailure(__FILE__, __LINE__,                        \
                                     #expression " threw: " +                   \
                                         std::string(ht_error.what()));         \
        }                                                                       \
    } while (false)
