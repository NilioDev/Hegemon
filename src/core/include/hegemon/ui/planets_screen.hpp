// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Planets screen (PLNTSUM.LBX) that the galaxy map's PLANETS button
// opens: the planets a colony or outpost ship could settle, sorted and
// filtered, with a small map, and a click that sends a ship there. See
// docs/rules/planets-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;
class Economy;

class PlanetsScreen {
public:
    // The pointers in this screen's colours: entry 7 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 7;
    static constexpr std::size_t kRowsShown = 8;

    enum class Sort : std::uint8_t { Climate, Minerals, Size };
    static constexpr std::size_t kSorts = 3;
    enum class Filter : std::uint8_t { NoEnemy, NormalGravity, NonHostile, Abundance, InRange };
    static constexpr std::size_t kFilters = 5;
    // Which ship a click on a planet sends.
    enum class Mode : std::uint8_t { None, Colony, Outpost };

    // Measured places of the buttons.
    static constexpr Rect kReturnButton = {454, 440, 156, 25};
    static constexpr Rect kColonyButton = {454, 386, 156, 23};
    static constexpr Rect kOutpostButton = {454, 413, 157, 25};
    static constexpr Rect kUpButton = {422, 16, 11, 19};
    static constexpr Rect kDownButton = {421, 448, 12, 19};
    static Rect sortRect(Sort sort);
    static Rect filterRect(Filter filter);
    // A row of the list, its whole width.
    static Rect rowRect(std::size_t row);

    // The planets `player` knows of. Throws FormatError when a file is
    // damaged.
    PlanetsScreen(GameData& data, const Game& game, const Economy& economy, int player);

    // What a row says about its planet.
    struct Row {
        int planet = 0;
        int star = 0;
        std::string name;
        std::string climate, food;
        std::string gravity, gravityNote;
        std::string minerals, production;
        std::string size, population;
        bool shipSent = false; // a ship was sent to settle it
    };
    // The rows in the order shown, filtered.
    const std::vector<Row>& rows() const { return shown_; }
    std::size_t top() const { return top_; }
    void scroll(int rows);
    Sort sort() const { return sort_; }
    void setSort(Sort sort);
    bool filter(Filter f) const { return filters_[static_cast<std::size_t>(f)]; }
    void setFilter(Filter f, bool on);
    Mode mode() const { return mode_; }
    // Whether the player has a ship for the mode.
    bool hasShip(Mode mode) const;

    // The planet chosen last, and what the box under the small map says:
    // how long the ship takes there, "3 turns to Sol".
    std::optional<int> chosen() const { return chosen_; }
    const std::string& turnsText() const { return turns_; }

    // The mouse. Send: the ship to send to the planet's star, and whether it
    // already had a course (then the original asks first).
    struct Action {
        enum class Kind : std::uint8_t { None, Return, Send };
        Kind kind = Kind::None;
        std::size_t ship = 0;
        int planet = 0;
        bool reroute = false;
    };
    void press(int x, int y);
    Action release(int x, int y);
    // A ship was sent to `planet`: its row shows the mark.
    void shipSent(int planet);

    // Hegemon's keyboard and gamepad: where the pointer goes from (x, y)
    // stepped by (dx, dy) over the rows and the buttons.
    std::pair<int, int> step(int x, int y, int dx, int dy) const;

    void draw(Canvas& canvas, Palette& palette) const;

private:
    struct Entry {
        Row row;
        int climate = 0, minerals = 0, size = 0;
        bool gravityNormal = true, hostile = false, enemy = false, inRange = false;
        std::size_t order = 0; // as the galaxy lists the planet
        Image icon;
    };
    void refresh();
    // The ship of the mode nearest to `star`, preferring one without a
    // course, and the turns it takes there.
    std::optional<std::pair<std::size_t, int>> nearestShip(Mode mode, int star) const;

    const Game* game_;
    const Parameters* parameters_;
    int player_;
    Image background_, return_, colony_, outpost_, up_, down_;
    std::array<Image, kSorts> sorts_;
    std::array<Image, kFilters> filterButtons_;
    std::vector<Image> starDots_; // PLNTSUM.LBX 15 to 25
    Image marker_;
    FontSet fonts_;
    std::vector<Entry> entries_;
    std::vector<std::size_t> order_; // entries shown
    std::vector<Row> shown_;
    struct Dot {
        int x = 0, y = 0;
        std::size_t entry = 0;
    };
    std::vector<Dot> stars_;
    std::array<bool, kFilters> filters_{false, false, false, false, true};
    Sort sort_ = Sort::Climate;
    Mode mode_ = Mode::None;
    std::size_t top_ = 0;
    std::optional<int> chosen_;
    std::string turnsPattern_, turnPattern_, turns_;
    Rect held_{};
};

} // namespace hegemon
