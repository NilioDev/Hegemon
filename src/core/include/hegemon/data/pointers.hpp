// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's mouse pointers: entries 1 to 13 of FONTS.LBX each hold a
// palette and 20 pointers drawn in its colours. See docs/formats/fonts.md.
#pragma once

#include "hegemon/data/image.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace hegemon {

class PointerSet {
public:
    static constexpr int kSize = 24;          // pointers are 24×24 pixels
    static constexpr std::size_t kCount = 20; // pointers in an entry
    static constexpr std::size_t kPaletteBytes = 1024;

    // Throws FormatError when the entry is too short.
    static PointerSet parse(std::span<const std::uint8_t> entry);

    // Draws pointer `index` with its top left corner (its hot spot) at
    // (x, y); colour 0 is see-through.
    void draw(std::size_t index, Canvas& canvas, int x, int y) const;

private:
    std::vector<std::uint8_t> pixels_;
};

} // namespace hegemon
