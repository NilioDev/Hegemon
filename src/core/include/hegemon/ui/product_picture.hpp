// SPDX-License-Identifier: GPL-3.0-or-later
//
// The picture of a product in the build list's box at the top and in the
// colony screen's box of what is built, as measured on the original. See
// docs/rules/colony-screen.md.
#pragma once

#include "hegemon/data/image.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace hegemon {

class GameData;

class ProductPicture {
public:
    enum class Box : std::uint8_t { BuildList, Colony };

    // The picture of `product` (rules/production.hpp names or the fan
    // patch's building table) for a player of stock race `race`; none for
    // a product without one; a design's picture is SHIPS.LBX entry
    // `shipImage`. Throws FormatError when a file is damaged.
    ProductPicture(GameData& data, std::string_view product, int race, int shipImage = -1);

    bool empty() const { return pixels_.empty(); }
    // Where its top left corner goes in a box; nothing when the box shows
    // none.
    std::optional<std::array<int, 2>> position(Box box) const;

    // Draws it in the box, its shadow darkening what is under it in the
    // colours of `palette`.
    void draw(Canvas& canvas, Box box, const Palette& palette) const;

private:
    int width_ = 0, height_ = 0;
    std::vector<std::uint8_t> pixels_; // colour indexes, row by row
    std::vector<bool> drawn_;          // which pixels the picture draws
    std::array<std::optional<std::array<int, 2>>, 2> at_{};
    // The colours it was drawn for, when not the screen's: it is shown in
    // the screen's colours nearest to them.
    std::optional<Palette> own_;
};

} // namespace hegemon
