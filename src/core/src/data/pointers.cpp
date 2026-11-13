// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/pointers.hpp"

#include "hegemon/data/reader.hpp"

namespace hegemon {

PointerSet PointerSet::parse(std::span<const std::uint8_t> entry) {
    constexpr std::size_t kBytes = kCount * kSize * kSize;
    if (entry.size() < kPaletteBytes + kBytes) {
        throw FormatError("pointer entry too short");
    }
    PointerSet set;
    const auto pixels = entry.subspan(kPaletteBytes, kBytes);
    set.pixels_.assign(pixels.begin(), pixels.end());
    return set;
}

void PointerSet::draw(std::size_t index, Canvas& canvas, int x, int y) const {
    if (index >= kCount) {
        throw FormatError("pointer index out of range");
    }
    const std::size_t base = index * kSize * kSize;
    for (int row = 0; row < kSize; ++row) {
        for (int col = 0; col < kSize; ++col) {
            const auto colour = pixels_[base + static_cast<std::size_t>(row * kSize + col)];
            const int px = x + col;
            const int py = y + row;
            if (colour != 0 && px >= 0 && py >= 0 && px < canvas.width() && py < canvas.height()) {
                canvas.set(px, py, colour);
            }
        }
    }
}

} // namespace hegemon
