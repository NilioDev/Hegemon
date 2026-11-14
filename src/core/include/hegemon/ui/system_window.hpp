// SPDX-License-Identifier: GPL-3.0-or-later
//
// The window that shows one star system from above: the star, the orbits
// with their planets, gas giants and asteroid belts, the wormhole there,
// the human's fleets there and Close. The galaxy map opens it at its top
// left corner when a star is clicked, and in the middle of the map when
// ships arrive. See docs/rules/star-system.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class SystemWindow {
public:
    enum class Place : std::uint8_t { MapCorner, Middle };
    enum class Action : std::uint8_t { None, Close };

    // The window for `star` as the human knows it, with `title` on top
    // (empty: "Star System %s", or "Star System Unexplored"). Throws
    // FormatError when a file is damaged.
    SystemWindow(GameData& data, const Game& game, int star, Place place, std::size_t banner,
                 const std::string& title = {});

    int star() const { return star_; }
    Place place() const { return place_; }
    const std::string& title() const { return title_; }
    const std::string& wormholeText() const { return wormhole_; }

    Rect area() const;
    Rect closeButton() const;
    // Where the disc of a planet or gas giant of the system is drawn, by
    // the galaxy's number of it; empty when it is not shown.
    std::optional<Rect> planetRect(int planet) const;
    // The planet or gas giant drawn at (x, y), if any.
    std::optional<int> planetAt(int x, int y) const;

    void press(int x, int y);
    Action release(int x, int y);
    void hover(int x, int y) { hoverX_ = x, hoverY_ = y; }
    // The planets turn as time goes by.
    void tick(std::uint64_t milliseconds) { time_ += milliseconds; }

    void draw(Canvas& canvas, Palette& palette) const;

    // Where on its orbit a planet is: degrees against the clock from the
    // right (Hegemon's choice; the original's rule is still to be found).
    static int planetAngle(int star, int orbit);

private:
    struct Body {
        int planet = -1;
        int x = 0, y = 0;  // the picture's top left corner
        int disc = 0;      // the width of the disc drawn
        std::size_t picture = 0; // in pictures_
        std::size_t shadow = 0;  // in shadows_
        std::size_t shadowFrame = 0;
    };

    int left() const;
    int top() const;

    int star_ = -1;
    Place place_ = Place::MapCorner;
    std::string title_, wormhole_;
    bool smallTitle_ = false;
    bool fleetHere_ = false;
    std::size_t banner_ = 0;
    Image frame_, close_, fleetIcon_, belt_;
    std::optional<Image> starPicture_;
    std::vector<Image> orbits_;   // the ellipses drawn
    std::vector<int> beltOrbits_; // frames of belt_
    std::vector<Image> pictures_;
    std::vector<Image> shadows_;
    std::vector<Body> bodies_;
    FontSet fonts_;
    Rect held_{};
    int hoverX_ = -1, hoverY_ = -1;
    std::uint64_t time_ = 0;
    // Each picture's colours on the screen at each shading level, for the
    // screen colours they were made for.
    mutable std::optional<Palette> shadedFor_;
    mutable std::vector<std::array<ColourMap, 10>> shades_;
};

} // namespace hegemon
