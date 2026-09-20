#pragma once

#include <SFML/Graphics/Color.hpp>

#include "core/Mark.hpp"

/// Every colour and measurement the interface uses, in one place.
///
/// Nothing here lives at global scope and no view file defines its own literal,
/// so changing the square size or the green is a one-line edit.
namespace ht::theme {

// -- Geometry -------------------------------------------------------------
inline constexpr float kMargin = 24.F;
inline constexpr float kNameColumnWidth = 200.F;
inline constexpr float kSquareSize = 38.F;
inline constexpr float kSquarePitch = 42.F;
inline constexpr float kRowPitch = 42.F;
inline constexpr float kOutlineThickness = 2.F;
inline constexpr float kDayHeaderHeight = 26.F;
inline constexpr float kNavBarGap = 22.F;
inline constexpr float kNavBarHeight = 44.F;
inline constexpr float kButtonWidth = 56.F;
inline constexpr float kButtonHeight = 44.F;
inline constexpr float kButtonGap = 12.F;

/// The window is sized to its content, but never beyond this.
inline constexpr unsigned kMaxWindowWidth = 1920;
inline constexpr unsigned kMaxWindowHeight = 1200;

// -- Type sizes -----------------------------------------------------------
inline constexpr unsigned kHabitCharacterSize = 19;
inline constexpr unsigned kDayCharacterSize = 12;
inline constexpr unsigned kLabelCharacterSize = 24;
inline constexpr unsigned kButtonCharacterSize = 24;

// -- Colours --------------------------------------------------------------
// sf::Color is not constexpr in SFML 2.5, so these are inline const rather than
// inline constexpr; they are still single definitions shared across the program.
inline const sf::Color kBackground{18, 20, 16};
inline const sf::Color kSquareOutline{58, 64, 49};
inline const sf::Color kTodayOutline{198, 176, 49};
inline const sf::Color kText{232, 234, 235};
inline const sf::Color kMutedText{134, 142, 121};
inline const sf::Color kButtonFill{46, 111, 64};
inline const sf::Color kButtonHoverFill{60, 138, 82};
inline const sf::Color kButtonText{240, 244, 238};

inline const sf::Color kMarkNone{28, 32, 24};
inline const sf::Color kMarkDone{46, 111, 64};
inline const sf::Color kMarkPartial{198, 176, 49};

/// The fill for a mark. Total over the Mark enumeration -- adding a Mark makes
/// this fail to compile rather than silently indexing out of a table.
[[nodiscard]] inline const sf::Color& markColor(Mark mark) noexcept {
    switch (mark) {
        case Mark::None: return kMarkNone;
        case Mark::Done: return kMarkDone;
        case Mark::Partial: return kMarkPartial;
    }
    return kMarkNone;
}

}  // namespace ht::theme
