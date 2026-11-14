// SPDX-License-Identifier: GPL-3.0-or-later
//
// The main screen: the galaxy map in its frame, as the original draws it
// when the whole galaxy is in view. See docs/rules/galaxy-map.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/galaxy.hpp"
#include "hegemon/ui/name_box.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;

class GalaxyMap {
public:
    // Where the map shows through the frame.
    static constexpr Rect kView = {20, 20, 508, 402};
    // The Turn button under the side panel (measured).
    static constexpr Rect kTurnButton = {547, 444, 58, 19};
    // The pointers in the game's colours: entry 1 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 1;

    // Reads the pictures. `banners` holds each player's banner colour (0
    // red to 7 orange), in the order of the galaxy's homes. Throws
    // FormatError when a file is damaged.
    GalaxyMap(GameData& data, const Galaxy& galaxy, std::vector<std::size_t> banners);

    // Where a point of the galaxy is on the screen.
    int screenX(int x) const;
    int screenY(int y) const;

    // What the side panel shows: the stardate (in tenths of a year) and the
    // human's empire.
    void setPanel(int stardate, const EmpireSummary& summary);
    // The stars where the human has ships.
    void setFleets(std::vector<int> stars) { fleets_ = std::move(stars); }

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    void drawPanel(Canvas& canvas) const;
    const Image& starPicture(const Star& s) const;

    const Galaxy& galaxy_;
    std::vector<std::size_t> banners_;
    Palette palette_;
    Image frame_;
    Image sky_;
    std::vector<Image> nebulae_;                   // by picture, at this scale
    std::vector<Image> starSizes_;                 // colour * 3 + size
    Image blackHole_;
    Image fleet_; // the fleet marker at this scale, a frame per banner
    std::vector<int> fleets_;
    std::string none_; // what the panel shows without a research project
    int stardate_ = 35000;
    EmpireSummary summary_;
    bool panel_ = false;
    FontSet fonts_;
    int scale_ = 1; // which of the pictures' four scales: the map size
};

// The Enter Home Star Name box over the map, offering `name`. Throws
// FormatError when a file is damaged.
NameBox homeStarNameBox(GameData& data, std::string name);

} // namespace hegemon
