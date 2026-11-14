// SPDX-License-Identifier: GPL-3.0-or-later
//
// The question the original asks before a colony ship lands: "Build colony
// on %s with" and what the planet gives the player's race there, with Yes
// and No. See docs/rules/colonizing.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;

class ColonizeBox {
public:
    enum class Answer : std::uint8_t { None, Yes, No };

    // For `player` landing on `planet`. Throws FormatError when a file is
    // damaged.
    ColonizeBox(GameData& data, const Game& game, int player, int planet);

    int planet() const { return planet_; }
    const std::string& title() const { return title_; }
    // Each row's words and value, as shown.
    const std::vector<std::pair<std::string, std::string>>& rows() const { return rows_; }

    static constexpr Rect kYes = {235, 302, 54, 24};
    static constexpr Rect kNo = {345, 302, 55, 24};

    void press(int x, int y);
    Answer release(int x, int y);

    // Draws the box over the screen, in the colours of `palette` nearest to
    // its own.
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    int planet_ = -1;
    std::string title_;
    std::vector<std::pair<std::string, std::string>> rows_;
    std::vector<Image> pictures_;
    FontSet fonts_;
    Rect held_{};
};

} // namespace hegemon
