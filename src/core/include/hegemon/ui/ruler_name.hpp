// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Enter Ruler Name dialog that follows the race selection, drawn and
// laid out as the original draws it. See docs/rules/ruler-name.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/pit_timer.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

// The names the original offers a stock race's ruler: entry 0 of
// RACENAME.LBX, eight for each race. See docs/formats/races.md.
class RulerNames {
public:
    static constexpr std::size_t kPerRace = 8;

    // Throws FormatError when the table is damaged or too short.
    explicit RulerNames(GameData& data);

    const std::string& name(std::size_t race, std::size_t choice) const;

private:
    std::vector<std::string> names_;
};

class RulerNameDialog {
public:
    // A name takes at most 19 characters, and a key adds one only while the
    // name is narrower than 100 pixels.
    static constexpr std::size_t kMaxLength = 19;
    static constexpr int kMaxWidth = 100;
    // The pointers in the race selection's colours: entry 10 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 10;

    // Reads the dialog's pictures and title. `race` is the button taken in
    // the race selection (0 to 13), whose portrait's colours the screen
    // keeps; `name` is the one offered. Throws FormatError when a file is
    // damaged.
    RulerNameDialog(GameData& data, std::size_t race, std::string name);

    const std::string& name() const { return name_; }

    // The keyboard: a key adds its character when the font has it and the
    // name has room; Backspace takes the last one away.
    bool type(char c);
    void backspace();

    // The mouse: Accept takes the name when the mouse button is let go over
    // it.
    Rect acceptRect() const;
    void press(int x, int y);
    bool release(int x, int y);
    bool click(int x, int y);

    // Lets `ms` milliseconds pass: the cursor after the name blinks.
    void tick(std::uint64_t ms);
    bool cursorShown() const { return cursorShown_; }

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    Image background_;
    Image dialog_;
    Palette palette_;   // the screen's colours
    ColourMap colours_; // the dialog's colours in the screen's
    FontSet fonts_;
    std::string title_;
    std::string name_;
    PitTimer timer_;
    bool cursorShown_ = true;
    bool held_ = false;
};

} // namespace hegemon
