#include "core/HabitName.hpp"

#include <stdexcept>

namespace ht {
namespace {

[[nodiscard]] bool isAsciiSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

/// Rejects C0 controls, DEL, and anything the on-disk format reserves. Bytes at
/// or above 0x80 are left alone so UTF-8 names pass through intact.
[[nodiscard]] bool isForbidden(char c) noexcept {
    const auto byte = static_cast<unsigned char>(c);
    return byte < 0x20 || byte == 0x7F;
}

}  // namespace

std::string_view trimmed(std::string_view text) noexcept {
    std::size_t begin = 0;
    while (begin < text.size() && isAsciiSpace(text[begin])) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && isAsciiSpace(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string HabitName::rejectionReason(std::string_view text) {
    const std::string_view candidate = trimmed(text);
    if (candidate.empty()) {
        return "a habit name cannot be empty";
    }
    if (candidate.size() > kMaxLength) {
        return "a habit name cannot be longer than " + std::to_string(kMaxLength) +
               " characters";
    }
    for (const char c : candidate) {
        if (isForbidden(c)) {
            return "a habit name cannot contain tabs or other control characters";
        }
    }
    return {};
}

bool HabitName::isValid(std::string_view text) noexcept {
    const std::string_view candidate = trimmed(text);
    if (candidate.empty() || candidate.size() > kMaxLength) {
        return false;
    }
    for (const char c : candidate) {
        if (isForbidden(c)) {
            return false;
        }
    }
    return true;
}

std::optional<HabitName> HabitName::tryMake(std::string_view text) {
    if (!isValid(text)) {
        return std::nullopt;
    }
    return HabitName(std::string(trimmed(text)), Validated{});
}

HabitName::HabitName(std::string_view text) : value_(trimmed(text)) {
    if (!isValid(text)) {
        throw std::invalid_argument("invalid habit name: " + rejectionReason(text));
    }
}

}  // namespace ht
