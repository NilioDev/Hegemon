// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Select New Research screen: the player's race scientist on the left
// and the eight fields that can be researched next, drawn as the original
// draws it. See docs/rules/research.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/pit_timer.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class ResearchSelectScreen {
public:
    // The eight boxes, row by row, left before right.
    static constexpr std::size_t kBoxes = 8;
    static constexpr std::array<TechCategory, kBoxes> kOrder = {
        TechCategory::Construction, TechCategory::Power,   TechCategory::Chemistry, TechCategory::Sociology,
        TechCategory::Computers,    TechCategory::Biology, TechCategory::Physics,   TechCategory::ForceFields};
    // The pointers in this screen's colours: entry 7 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 7;

    // Reads the pictures for the race (0 to 12, as Races counts them) and
    // offers what the player can research next. Throws FormatError when a
    // file is damaged.
    ResearchSelectScreen(GameData& data, const TechTree& tree, const Knowledge& knowledge, int race, int pointerX,
                         int pointerY);

    Rect boxRect(std::size_t box) const;
    std::optional<std::size_t> boxAt(int x, int y) const;
    // The field in a box, 0 when the category has none left.
    int field(std::size_t box) const { return boxes_[box].field; }
    // The box drawn lit, and the application the arrow points at in it.
    std::optional<std::size_t> highlighted() const { return highlighted_; }
    std::optional<std::size_t> pointedApp() const { return app_; }

    // The mouse. A field is chosen when the button is let go over its box;
    // the result is the field's id.
    void pointerMoved(int x, int y);
    void press(int x, int y);
    std::optional<int> release(int x, int y);
    std::optional<int> click(int x, int y);
    // Hegemon's gamepad and keyboard: light the next box with a field.
    void moveHighlight(int step);
    // Hegemon's gamepad and keyboard: point at the next application of the
    // lit box.
    void moveApp(int step);
    // The application chosen with the field: the one pointed at, else the
    // lit box's first (assumed); 0 with no box lit.
    int chosenTech() const;
    // The lit box's field, if any (the gamepad's A button).
    std::optional<int> choose() const;

    // Lets `ms` milliseconds pass: the scientist's animation and the lit
    // box's border.
    void tick(std::uint64_t ms);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    struct Box {
        int field = 0;
        std::string name, cost;
        std::vector<std::string> apps;
        std::vector<int> techs; // the applications' ids
    };
    int nameY(std::size_t box) const;

    std::array<Box, kBoxes> boxes_;
    Image scientist_; // the race's animation, a whole screen
    Image panel_;
    Image textColours_; // the entry that carries the text's colours
    Palette base_;
    FontSet fonts_;
    Canvas animation_;
    std::size_t frame_ = 0;
    std::uint64_t ticks_ = 0;
    PitTimer timer_;
    std::optional<std::size_t> highlighted_;
    std::optional<std::size_t> app_;
    bool held_ = false;
};

} // namespace hegemon
