// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Fleet Operations screen (FLEET.LBX) that the galaxy map's FLEETS
// button opens: one of the player's fleets at a time, its ships in a grid,
// what the chosen ship carries, and a small map. See
// docs/rules/fleets-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/fleet.hpp"
#include "hegemon/ui/part_names.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;
class TechTree;

class FleetsScreen {
public:
    // The pointers in the game's colours, as on the galaxy map.
    static constexpr std::size_t kPointerEntry = 1;
    static constexpr int kColumns = 4, kRows = 5;

    // Measured places of the buttons.
    static constexpr Rect kReturnButton = {556, 430, 73, 27};
    static constexpr Rect kLeadersButton = {342, 430, 73, 27};
    static constexpr Rect kSupportButton = {425, 435, 61, 19};
    static constexpr Rect kCombatButton = {487, 435, 60, 19};
    static constexpr Rect kAllButton = {348, 380, 74, 28};
    static constexpr Rect kRelocateButton = {441, 380, 89, 28};
    static constexpr Rect kScrapButton = {549, 380, 73, 28};
    static constexpr Rect kPreviousButton = {19, 249, 32, 19};
    static constexpr Rect kNextButton = {283, 249, 31, 19};
    static constexpr Rect kUpButton = {606, 59, 13, 24};
    static constexpr Rect kDownButton = {605, 325, 14, 25};
    // A cell of the grid, counted from the top left of what is shown.
    static Rect cellRect(std::size_t cell);

    // The fleets of `player`, the first shown. Throws FormatError when a
    // file is damaged.
    FleetsScreen(GameData& data, const Game& game, const TechTree& tree, int player);

    // The fleets, and the one shown.
    const std::vector<Fleet>& fleets() const { return fleets_; }
    std::size_t fleet() const { return fleet_; }
    void showFleet(std::size_t fleet);
    // What the bar under the map says: "ALKARI FLEET: 1 Colony Ship, 2
    // Frigates".
    const std::string& title() const { return title_; }
    // The ships in the grid, in the game's list, after the filters.
    const std::vector<std::size_t>& shown() const { return shown_; }
    std::size_t top() const { return top_; }
    void scroll(int rows);
    bool support() const { return support_; }
    bool combat() const { return combat_; }
    void setSupport(bool on);
    void setCombat(bool on);

    // The ships chosen (lit), in the game's list; the one clicked last,
    // framed, and what the panel says of it: the lines of a warship, or the
    // help text of a colony ship.
    const std::vector<std::size_t>& selected() const { return selected_; }
    std::optional<std::size_t> chosen() const { return chosen_; }
    struct Line {
        int x = 0, y = 0;
        std::size_t font = 0;
        int spacing = 0;
        std::string text;
        bool right = false; // x is where it ends
    };
    const std::vector<Line>& panel() const { return panel_; }

    enum class Action : std::uint8_t { None, Return };
    void press(int x, int y);
    Action release(int x, int y);

    // Hegemon's keyboard and gamepad: where the pointer goes from (x, y)
    // stepped by (dx, dy) over the cells and the buttons.
    std::pair<int, int> step(int x, int y, int dx, int dy) const;

    void draw(Canvas& canvas, Palette& palette) const;

private:
    void refresh();
    void describe();
    std::vector<Rect> targets() const;

    GameData* data_;
    const Game* game_;
    int player_;
    std::size_t banner_ = 0;
    Palette palette_;
    Image background_, scrollBar_, up_, down_, previous_, next_, all_, relocate_, scrap_, scrapOff_, leadersOff_,
        supportButton_, combatButton_, return_, lit_, box_, marker_, shipColours_;
    std::vector<Image> starDots_; // FLEET.LBX 34 to 44
    std::vector<Image> pictures_; // of each ship in the game's list, for the player's banner
    FontSet fonts_;
    std::vector<std::string> strings_; // HESTRNGS.LBX
    std::string race_;
    PartNames names_;
    std::string colonyHelp_, outpostHelp_, transportHelp_; // HELP.LBX
    std::vector<Fleet> fleets_;
    std::size_t fleet_ = 0;
    std::vector<std::size_t> shown_;
    std::size_t top_ = 0;
    bool support_ = true, combat_ = true;
    std::vector<std::size_t> selected_;
    std::optional<std::size_t> chosen_;
    std::string title_;
    std::vector<Line> panel_;
    struct Dot {
        int x = 0, y = 0;
        std::size_t entry = 0;
    };
    std::vector<Dot> stars_;
    Rect held_{};
};

} // namespace hegemon
