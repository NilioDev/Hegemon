// SPDX-License-Identifier: GPL-3.0-or-later
//
// The window over the galaxy map's top left corner that shows one of the
// human's fleets: its ships in boxes of three a row, which of them are
// chosen, where the fleet is or when it arrives, and Close. While it is
// open a star on the map is where the chosen ships go. See
// docs/rules/fleets.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/fleet.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class FleetWindow {
public:
    enum class Action : std::uint8_t { None, Close, Colonize };

    // The fleet's ships, all chosen. Throws FormatError when a file is
    // damaged.
    FleetWindow(GameData& data, const Game& game, const Fleet& fleet, std::size_t banner);

    // The ships it shows, in the game's list, and those chosen.
    const std::vector<std::size_t>& ships() const { return ships_; }
    std::vector<std::size_t> chosen() const;
    // What the line beside All says, from the game as it is now.
    const std::string& status() const { return status_; }
    // After the chosen ships were sent off: they are what it shows now.
    void update(const Game& game, const Fleet& fleet);

    // Where things are.
    Rect area() const;
    Rect shipBox(std::size_t index) const;
    Rect allButton() const;
    Rect closeButton() const;
    // Colonize Planet, under Close, for a fleet with a colony ship at a star
    // with a planet to settle; empty otherwise.
    std::optional<Rect> colonizeButton() const;

    // The mouse: a box changes whether its ship is chosen, All chooses
    // them all, Close closes, Colonize Planet asks where to land.
    void press(int x, int y);
    Action release(int x, int y);
    void hover(int x, int y) { hoverX_ = x, hoverY_ = y; }

    // Draws the window over the map, with the ship colours of its banner.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    int rows() const;
    int bottom() const;
    void describe(const Game& game, const Fleet& fleet);

    std::vector<std::size_t> ships_;
    std::vector<bool> chosen_;
    std::vector<Image> pictures_; // of each ship, for its banner
    std::string title_, status_;
    std::vector<std::string> strings_; // HESTRNGS.LBX
    Image top_, grid_, foot_, box_, all_, close_, colonize_;
    bool canColonize_ = false;
    Image shipColours_;
    FontSet fonts_;
    Rect held_{};
    int hoverX_ = -1, hoverY_ = -1;
};

} // namespace hegemon
