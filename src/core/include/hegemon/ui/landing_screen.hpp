// SPDX-License-Identifier: GPL-3.0-or-later
//
// What the original shows when a colony ship lands: the planet's landscape
// with the ship coming down, "Colony built on %s", then what the planet is
// like. See docs/rules/colonizing.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class LandingScreen {
public:
    // For the new colony `colony` of the game. Throws FormatError when a
    // file is damaged.
    LandingScreen(GameData& data, const Game& game, std::size_t colony);

    const std::string& title() const { return title_; }
    // The lines about the planet, shown once the ship is down.
    const std::vector<std::string>& lines() const { return lines_; }
    // Whether the ship is down.
    bool landed() const;

    void tick(std::uint64_t milliseconds) { time_ += milliseconds; }
    // The ship lands at once (a click while it comes down, Hegemon's choice).
    void skip();

    // The screen's colours, the colony screen's.
    const Palette& palette() const { return palette_; }
    void draw(Canvas& canvas, Palette& palette) const;

private:
    std::size_t frame() const;

    Image sky_, landscape_, landing_;
    FontSet fonts_;
    Palette palette_;
    std::array<std::uint8_t, 256> shade_{};
    std::string title_;
    std::vector<std::string> lines_;
    std::uint64_t time_ = 0;
};

} // namespace hegemon
