// SPDX-License-Identifier: GPL-3.0-or-later
//
// The box the original asks for a name with (Enter Ruler Name, Enter Home
// Star Name): INBOX.LBX's picture with a title, the name being typed with a
// blinking cursor, and Accept. See docs/rules/ruler-name.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/pit_timer.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace hegemon {

class GameData;

class NameBox {
public:
    // A name takes at most 19 characters, and a key adds one only while the
    // name is narrower than 100 pixels.
    static constexpr std::size_t kMaxLength = 19;
    static constexpr int kMaxWidth = 100;

    // The colours of the title's three shades and the name's two.
    struct Shades {
        std::array<std::uint8_t, 3> title;
        std::array<std::uint8_t, 2> name;
    };

    // Reads the box's picture; its top left corner goes at (x, y). Throws
    // FormatError when a file is damaged.
    NameBox(GameData& data, std::string title, std::string name, int x, int y, Shades shades);

    // The box was drawn for the colours of FONTS.LBX entry 1; on a screen
    // with other colours it is drawn in these (the box's colours in the
    // screen's), its text too.
    void useColours(const ColourMap& colours) { colours_ = colours; }
    // The box's own colours.
    const Image& picture() const { return picture_; }

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

    void draw(Canvas& canvas) const;

private:
    Image picture_;
    ColourMap colours_;
    FontSet fonts_;
    std::string title_;
    std::string name_;
    int x_, y_;
    Shades shades_;
    PitTimer timer_;
    bool cursorShown_ = true;
    bool held_ = false;
};

} // namespace hegemon
