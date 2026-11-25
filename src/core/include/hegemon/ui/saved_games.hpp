// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's Load Game and Save Game windows (GAME.LBX): the ten saved
// games, over the main menu or the galaxy map; the warning box
// (WARNING.LBX) for a slot that holds none; and the game menu the galaxy
// map's GAME button opens. See docs/rules/saved-games.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/pit_timer.hpp"
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
    int stardate = 0; // in tenths of a year
    std::string date; // when it was saved, as "Nov 25, 2026"
    std::string time; // and "02:34"
};

// When a saved game was saved, as the window shows it; a month from 1 to 12.
std::string savedDate(int year, int month, int day);
std::string savedTime(int hour, int minute);

class SavedGamesWindow {
public:
    // SAVE1.GAM to SAVE9.GAM, then the autosave, SAVE10.GAM.
    static constexpr std::size_t kSlots = 10;
    using Slots = std::array<std::optional<SaveSlot>, kSlots>;

    enum class Kind : std::uint8_t { Load, Save };
    // Over the main menu (only to load) or over the galaxy map.
    enum class Over : std::uint8_t { MainMenu, Game };

    // A name being typed takes a character only while narrower than this.
    static constexpr int kMaxNameWidth = 200;

    // `last` is the slot loaded or saved last, shown brighter, if any.
    // Throws FormatError when a file is damaged.
    SavedGamesWindow(GameData& data, Kind kind, Over over, Slots slots, std::optional<std::size_t> last);

    Kind kind() const { return kind_; }
    Rect slotRect(std::size_t slot) const;
    Rect okRect() const; // Load or Save
    Rect cancelRect() const;

    struct Result {
        enum class Kind : std::uint8_t { None, Load, Save, Cancel };
        Kind kind = Kind::None;
        std::size_t slot = 0; // to load (it may hold no saved game) or save
        std::string name;     // to save it under
    };

    // The mouse. Loading: a click on a slot loads it at once; Load loads the
    // slot used last, when there is one. Saving: a click on a slot starts
    // typing a name there, beginning with what the slot shows.
    Result press(int x, int y);
    Result release(int x, int y);

    // The keyboard while a name is typed.
    std::optional<std::size_t> editing() const { return editing_; }
    const std::string& name() const { return name_; }
    bool type(char c);
    void backspace();
    Result enter();

    // Lets time pass: the cursor after the name blinks.
    void tick(std::uint64_t ms);

    // In the colours of `palette` nearest to its own.
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    Kind kind_;
    Over over_;
    Image window_, ok_, cancel_, player_;
    Palette own_;
    FontSet fonts_;
    Slots slots_;
    std::optional<std::size_t> last_;
    std::optional<std::size_t> editing_;
    std::string name_;
    PitTimer timer_;
    bool cursorShown_ = true;
    Rect held_{};
};

// The warning box with its blinking lights over a screen, such as
// "SAVE3.GAM does not exist"; a click anywhere closes it.
class WarningBox {
public:
    WarningBox(GameData& data, std::string text);
    // The galaxy map's message of an attack, "A Space Hydra attacks Alkari
    // at the Ell system": the same box, the text in red, wrapped to lines.
    static WarningBox attack(GameData& data, std::string text);

    const std::vector<std::string>& lines() const { return lines_; }

    const std::string& text() const { return text_; }
    // Lets time pass for the lights.
    void advance(std::uint64_t ms);
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    Image box_;
    FontSet fonts_;
    std::string text_;
    std::vector<std::string> lines_;
    std::array<std::uint8_t, 3> shades_{};
    std::uint64_t micros_ = 0;
};

// The game menu over the galaxy map (GAME.LBX 0): Save Game, Load Game, New
// Game, Quit Game, the music and sound volumes, Settings and Return.
class GameMenu {
public:
    enum class Button : std::uint8_t { Save, Load, NewGame, Quit, Settings, Return };
    static constexpr std::size_t kButtons = 6;

    explicit GameMenu(GameData& data);

    static Rect buttonRect(Button b);

    // A button when the mouse button is let go over the one pressed.
    void press(int x, int y);
    std::optional<Button> release(int x, int y);

    void draw(Canvas& canvas) const;

private:
    Image menu_;
    std::array<Image, kButtons> buttons_;
    Image volume_;
    std::optional<Button> held_;
};

} // namespace hegemon
