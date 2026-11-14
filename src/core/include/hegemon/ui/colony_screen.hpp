// SPDX-License-Identifier: GPL-3.0-or-later
//
// The colony screen: the planet's landscape under its sky, the panels at
// the top and the buttons, drawn as the original draws them. See
// docs/rules/colony-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <string>
#include <vector>

namespace hegemon {

class GameData;
class Economy;

class ColonyScreen {
public:
    // The pointers in this screen's colours: entry 2 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 2;
    // Measured: the Return button at the bottom right.
    static constexpr Rect kReturnButton = {552, 457, 77, 22};

    // Reads the pictures for a colony of the game. Throws FormatError when a
    // file is damaged.
    ColonyScreen(GameData& data, const Game& game, const Economy& economy, std::size_t colony);

    const std::string& title() const { return title_; }
    const std::string& population() const { return population_; }

    // The rows of icons at the top: money, food, production and research.
    // Each holds COLONY2.LBX entries from the left, kGap for an empty place.
    static constexpr int kGap = -1;
    enum Row : std::size_t { Money, Food, Production, Research, kRows };
    const std::vector<int>& row(Row r) const { return rows_.at(r); }
    // How far apart a row's icons are drawn.
    static int iconStep(std::size_t places);

    // The mouse: true when Return is let go over.
    void press(int x, int y);
    bool release(int x, int y);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    Palette base_;
    Image sky_;
    Image landscape_;
    Image frame_;
    std::vector<Image> icons_; // COLONY2.LBX entries 0 to 26
    std::array<Image, 3> people_; // a farmer, a worker and a scientist
    std::array<std::vector<int>, kRows> rows_;
    std::array<int, 3> jobs_{};
    int government_ = 0;
    FontSet fonts_;
    std::string title_;
    std::string population_;
    bool held_ = false;
};

} // namespace hegemon
