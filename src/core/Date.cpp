#include "core/Date.hpp"

#include <ctime>

namespace ht {
namespace {

/// Thread-safe localtime, spelled differently on every platform.
[[nodiscard]] bool localTime(std::time_t stamp, std::tm& out) noexcept {
#if defined(_WIN32)
    return ::localtime_s(&out, &stamp) == 0;
#else
    return ::localtime_r(&stamp, &out) != nullptr;
#endif
}

}  // namespace

YearMonthDay currentDate() noexcept {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (now == static_cast<std::time_t>(-1) || !localTime(now, local)) {
        return YearMonthDay{};
    }

    const int year = local.tm_year + 1900;
    if (!isYearInRange(year) || local.tm_mon < 0 ||
        local.tm_mon >= static_cast<int>(kMonthsPerYear) || local.tm_mday < 1) {
        return YearMonthDay{};
    }
    const auto month = static_cast<Month>(static_cast<unsigned>(local.tm_mon));
    const auto day = static_cast<unsigned>(local.tm_mday - 1);
    if (day >= daysInMonth(year, month)) {
        return YearMonthDay{};
    }
    return YearMonthDay{year, month, day};
}

}  // namespace ht
