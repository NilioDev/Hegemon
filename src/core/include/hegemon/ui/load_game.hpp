// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's Load Game window (GAME.LBX) over the main menu: the ten
// saved games, a click on one loads it, and the warning box (WARNING.LBX)
// for a slot that holds none. See docs/rules/saved-games.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

// What a slot of the window shows of the saved game in it.
struct SaveSlot {
    std::string name;
    int stardate = 0;  // in tenths of a year
    std::string date;  // when it was saved, as "Nov 25, 2026"
    std::string time;  // and "02:34"
};

// When a saved game was saved, as the window shows it; a month from 1 to 12.
std::string savedDate(int year, int month, int day);
std::string savedTime(int hour, int minute);

class LoadGameWindow {
public:
    // SAVE1.GAM to SAVE9.GAM, then the autosave, SAVE10.GAM.
    static constexpr std::size_t kSlots = 10;
    using Slots = std::array<std::optional<SaveSlot>, kSlots>;

    // `last` is the slot loaded or saved last, shown brighter, if any.
    // Throws FormatError when a file is damaged.
    LoadGameWindow(GameData& data, Slots slots, std::optional<std::size_t> last);

    static Rect slotRect(std::size_t slot);
    static constexpr Rect kLoad = {217, 388, 68, 22};
    static constexpr Rect kCancel = {351, 389, 68, 21};

    struct Result {
        enum class Kind : std::uint8_t { None, Load, Cancel };
        Kind kind = Kind::None;
        std::size_t slot = 0; // to load: it may hold no saved game
    };

    // The mouse: a click on a slot loads it at once; Load loads the slot
    // used last, when there is one.
    Result press(int x, int y);
    Result release(int x, int y);

    // Over the main menu, in the colours of `palette` nearest to its own.
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    Image window_, load_, cancel_, player_;
    Palette own_;
    FontSet fonts_;
    Slots slots_;
    std::optional<std::size_t> last_;
    Rect held_{};
};

// The warning box with its blinking lights over a screen, such as
// "SAVE3.GAM does not exist"; a click anywhere closes it.
class WarningBox {
public:
    WarningBox(GameData& data, std::string text);

    const std::string& text() const { return text_; }
    // Lets time pass for the lights.
    void advance(std::uint64_t ms);
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    Image box_;
    FontSet fonts_;
    std::string text_;
    std::uint64_t micros_ = 0;
};

} // namespace hegemon
