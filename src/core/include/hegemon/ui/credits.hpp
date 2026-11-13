// SPDX-License-Identifier: GPL-3.0-or-later
//
// The credits that scroll up on the left of the main menu, timed and laid
// out as the original's (docs/rules/main-menu.md).
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/pit_timer.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;

class Credits {
public:
    // Reads the lines from CREDITS.LBX and the font from FONTS.LBX.
    explicit Credits(GameData& data);

    // Lets `ms` milliseconds pass.
    void tick(std::uint64_t ms);
    // Lets ticks of the PC timer pass.
    void advanceTicks(std::uint64_t ticks);
    void draw(Canvas& canvas) const;

    // How far the lines have moved up since they last started, in pixels.
    int offset() const { return offset_; }
    // Moves the lines to an offset, for tests and comparing with captures.
    void setOffset(int pixels) { offset_ = pixels; }
    // Where line `i` starts, from the top of the screen.
    int lineTop(std::size_t i) const;

    // The fonts the credits were read with, for the rest of the menu.
    const FontSet& fonts() const { return fonts_; }

private:
    FontSet fonts_;
    std::vector<std::pair<std::string, std::string>> lines_;
    int offset_ = 0;
    PitTimer timer_;
    std::uint64_t ticksDone_ = 0;
};

} // namespace hegemon
