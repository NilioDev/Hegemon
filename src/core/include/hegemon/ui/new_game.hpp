// SPDX-License-Identifier: GPL-3.0-or-later
//
// The New Game screen, where the player picks the difficulty, the galaxy
// and the other settings of a new game, drawn and laid out as the original
// draws it. See docs/rules/new-game.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace hegemon {

class GameData;

// The choices of the New Game screen. Each list is in the order a click
// steps through it.
struct NewGameSettings {
    enum class Difficulty : std::uint8_t { Tutor, Easy, Average, Hard, Impossible };
    enum class GalaxySize : std::uint8_t { Small, Medium, Large, Cluster, Huge };
    enum class GalaxyAge : std::uint8_t { MineralRich, Average, OrganicRich };
    enum class TechLevel : std::uint8_t { PreWarp, Average, PostWarp, Advanced };
    // Players 2 to 8, or a random number of computer players (fan patch 1.51).
    static constexpr int kRandomPlayers = 0;
    static constexpr int kMinPlayers = 2;
    static constexpr int kMaxPlayers = 8;

    // What the fan patch shows the first time.
    Difficulty difficulty = Difficulty::Tutor;
    GalaxySize galaxySize = GalaxySize::Medium;
    GalaxyAge galaxyAge = GalaxyAge::Average;
    int players = 5;
    TechLevel techLevel = TechLevel::Average;
    bool tacticalCombat = true;
    bool randomEvents = true;
    bool antaransAttack = true;

    bool operator==(const NewGameSettings&) const = default;
};

class NewGameScreen {
public:
    // What can be clicked, in the order Hegemon's gamepad steps through it.
    enum class Item : std::uint8_t {
        Difficulty,
        GalaxySize,
        GalaxyAge,
        Players,
        TechLevel,
        TacticalCombat,
        RandomEvents,
        AntaransAttack,
        Cancel,
        Accept,
    };
    static constexpr std::size_t kItems = 10;
    // The pointers in this screen's colours: entry 10 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 10;
    enum class Result : std::uint8_t { Cancel, Accept };

    // Reads the screen's pictures. Throws FormatError when a file is damaged.
    NewGameScreen(GameData& data, const NewGameSettings& settings);

    const NewGameSettings& settings() const { return settings_; }

    // Where an item can be clicked, and the item at (x, y), if any.
    Rect itemRect(Item item) const;
    std::optional<Item> itemAt(int x, int y) const;

    // The mouse. As in the original, a switch flips when the mouse button
    // goes down on it; the pictures step on and the buttons are taken when
    // it is let go over them. While it is held down, Cancel and Accept look
    // pressed when the pointer is over them.
    void pointerMoved(int x, int y);
    void press(int x, int y);
    std::optional<Result> release(int x, int y);
    // A press and release at the same place; the original's Enter key does
    // this where the pointer is.
    std::optional<Result> click(int x, int y);
    // The original's Esc key: the same as Cancel.
    static Result escape() { return Result::Cancel; }

    // The text shown under a picture.
    std::string_view valueText(Item item) const;

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    void step(Item item);
    std::size_t pictureEntry(Item item) const;

    NewGameSettings settings_;
    Image background_;
    Image panel_;
    std::vector<Image> pictures_; // entries 1 to 25 of NEWGAME.LBX
    std::array<Image, 3> switches_;
    std::array<Image, 2> buttons_; // Cancel and Accept
    FontSet fonts_;
    bool held_ = false;
    int pointerX_ = 320, pointerY_ = 240;
};

} // namespace hegemon
