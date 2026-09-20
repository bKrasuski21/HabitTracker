#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <string_view>

namespace ht {

/// A habit name that is valid by construction.
///
/// There is no way to obtain a HabitName holding a value that would corrupt the
/// save file or render as an empty row: it is non-empty, trimmed, bounded in
/// length, and free of control characters (which includes the tab used as the
/// field separator on disk). Every layer above this can therefore treat a
/// HabitName as unconditionally safe to write, compare, and display.
class HabitName final {
public:
    /// Longest accepted name. Bounded so a corrupt file cannot make the app
    /// allocate without limit, and so names stay renderable in the name column.
    static constexpr std::size_t kMaxLength = 64;

    /// Constructs from `text`, throwing std::invalid_argument if it is not a
    /// valid name. Use tryMake() on any path where invalid input is expected
    /// rather than exceptional -- parsing a file, or reading from a user.
    explicit HabitName(std::string_view text);

    /// Returns the name built from `text`, or nullopt if `text` is not valid.
    /// Surrounding whitespace is trimmed before validation.
    [[nodiscard]] static std::optional<HabitName> tryMake(std::string_view text);

    /// True when tryMake(text) would succeed.
    [[nodiscard]] static bool isValid(std::string_view text) noexcept;

    /// Human-readable reason `text` is rejected, or an empty string if it is
    /// valid. Used to tell the user what to type instead.
    [[nodiscard]] static std::string rejectionReason(std::string_view text);

    [[nodiscard]] const std::string& str() const noexcept { return value_; }
    [[nodiscard]] std::size_t size() const noexcept { return value_.size(); }

    [[nodiscard]] friend bool operator==(const HabitName& lhs,
                                         const HabitName& rhs) noexcept {
        return lhs.value_ == rhs.value_;
    }
    [[nodiscard]] friend bool operator<(const HabitName& lhs,
                                        const HabitName& rhs) noexcept {
        return lhs.value_ < rhs.value_;
    }

private:
    struct Validated {};
    HabitName(std::string value, Validated) noexcept : value_(std::move(value)) {}

    std::string value_;
};

/// Removes leading and trailing ASCII whitespace.
[[nodiscard]] std::string_view trimmed(std::string_view text) noexcept;

}  // namespace ht
