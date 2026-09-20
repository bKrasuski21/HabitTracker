#include "TestHarness.hpp"

namespace testing {
namespace {
int gCurrentFailures = 0;
}

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

int& currentFailures() { return gCurrentFailures; }

void reportFailure(const char* file, int line, const std::string& message) {
    ++gCurrentFailures;
    std::cerr << "    " << file << ":" << line << ": " << message << '\n';
}

int runAll() {
    int failed = 0;
    for (const TestCase& test : registry()) {
        gCurrentFailures = 0;
        try {
            test.function();
        } catch (const std::exception& e) {
            reportFailure(__FILE__, __LINE__,
                          std::string("unexpected exception: ") + e.what());
        } catch (...) {
            reportFailure(__FILE__, __LINE__, "unexpected non-standard exception");
        }
        if (gCurrentFailures > 0) {
            ++failed;
            std::cerr << "FAIL  " << test.name << " (" << gCurrentFailures
                      << " check" << (gCurrentFailures == 1 ? "" : "s") << ")\n";
        } else {
            std::cout << "ok    " << test.name << '\n';
        }
    }

    const std::size_t total = registry().size();
    if (failed == 0) {
        std::cout << "\n" << total << " test" << (total == 1 ? "" : "s") << " passed.\n";
    } else {
        std::cerr << "\n" << failed << " of " << total << " tests failed.\n";
    }
    return failed;
}

}  // namespace testing

int main() { return testing::runAll() == 0 ? 0 : 1; }
