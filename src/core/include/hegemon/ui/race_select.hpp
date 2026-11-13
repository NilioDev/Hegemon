// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Select Race screen that follows the New Game screen, drawn and laid
// out as the original draws it. See docs/rules/race-select.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class RaceSelectScreen {
public:
    // The thirteen stock races, then Custom.
    static constexpr std::size_t kButtons = Races::kStockRaces + 1;
    static constexpr std::size_t kCustom = Races::kStockRaces;
    // The pointers in this screen's colours: entry 10 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 10;

    // Reads the screen's pictures. Throws FormatError when a file is damaged.
    // `pointerX` and `pointerY` are where the pointer is when it opens.
    RaceSelectScreen(GameData& data, const Races& races, int pointerX, int pointerY);

    Rect buttonRect(std::size_t button) const;
    std::optional<std::size_t> buttonAt(int x, int y) const;

    // The button the portrait and text are for: the last one the pointer
    // was over.
    std::size_t highlighted() const { return highlighted_; }
    // The text under the portrait, broken into lines.
    const std::vector<std::string>& descriptionLines() const { return lines_; }

    // The mouse. A button is taken when the mouse button is let go over it:
    // a race (0 to 12) or Custom (13).
    void pointerMoved(int x, int y);
    void press(int x, int y);
    std::optional<std::size_t> release(int x, int y);
    std::optional<std::size_t> click(int x, int y);
    // Hegemon's gamepad and keyboard: move the highlight through the
    // buttons, column by column.
    void moveHighlight(int step);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    void highlight(std::size_t button);

    const Races& races_;
    Image background_;
    Image panel_;
    Image title_;
    std::vector<Image> buttons_;   // two frames: normal, then the lit text
    std::vector<Image> portraits_; // one per button
    FontSet fonts_;
    std::size_t highlighted_ = 0;
    std::vector<std::string> lines_;
    bool held_ = false;
};

} // namespace hegemon
