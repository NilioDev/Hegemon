// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pictures and animations from LBX entries, and the 256-colour palettes they
// are drawn with. See docs/formats/images.md.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace hegemon {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    bool operator==(const Color&) const = default;
};

// 256 colours. The original stores 6 bits per channel (0 to 63); they are
// widened to 8 bits here so that 63 becomes 255.
struct Palette {
    std::array<Color, 256> colors{};

    // Sets `count` colours from `first` on, from the original's 4-byte
    // entries (one unused byte, then red, green and blue).
    void set(std::size_t first, std::span<const std::uint8_t> entries);
};

// For each colour of one palette, the colour of another to draw it in.
using ColourMap = std::array<std::uint8_t, 256>;

// Maps every colour of `from` to the colour of `to` nearest to it, as the
// original does when it shows a picture made for other colours: the least
// sum of the squared differences of the 6-bit red, green and blue, the
// lowest number on a tie, and never colour 0. See docs/formats/images.md.
ColourMap nearestColours(const Palette& from, const Palette& to);

// A picture of 8-bit colour indexes. Pixels a frame does not draw keep the
// value they had, which is how the original draws transparent parts and
// builds an animation frame on top of the previous one.
class Canvas {
public:
    Canvas(int width, int height, std::uint8_t fill = 0);

    int width() const { return width_; }
    int height() const { return height_; }
    std::uint8_t at(int x, int y) const { return pixels_[index(x, y)]; }
    void set(int x, int y, std::uint8_t value) { pixels_[index(x, y)] = value; }
    std::span<const std::uint8_t> pixels() const { return pixels_; }

private:
    std::size_t index(int x, int y) const {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x);
    }

    int width_;
    int height_;
    std::vector<std::uint8_t> pixels_;
};

// Turns the colour indexes into RGBA bytes, four per pixel, for showing on
// screen. `out` must hold width × height × 4 bytes.
void toRgba(const Canvas& canvas, const Palette& palette, std::span<std::uint8_t> out);

class Image {
public:
    // Flag bit: the frames are plain rows of pixels, not runs.
    static constexpr std::uint16_t kFlagRaw = 0x0100;
    // Flag bit: the entry carries colours for the palette.
    static constexpr std::uint16_t kFlagPalette = 0x1000;

    // Checks the header, the frame offsets and every frame against the size
    // of the entry and the size of the picture; throws FormatError when
    // anything is out of place. Keeps its own copy of the bytes.
    static Image parse(std::span<const std::uint8_t> entry);

    int width() const { return width_; }
    int height() const { return height_; }
    std::size_t frameCount() const { return frames_.size(); }
    std::uint16_t flags() const { return flags_; }
    // The header word after the frame count. In animations it is the number
    // of a frame (the opening's 50 frames give 49); its meaning is still to
    // be measured.
    std::uint16_t loopFrame() const { return loopFrame_; }

    // Colours the entry carries, if any: `first` and the colours from there.
    struct PaletteChange {
        std::size_t first = 0;
        std::vector<Color> colors;
    };
    const std::optional<PaletteChange>& paletteChange() const { return palette_; }
    void applyPalette(Palette& palette) const;

    // Draws frame `index` with its top left corner at (x, y). Pixels that fall
    // outside the canvas are left out.
    void draw(std::size_t index, Canvas& canvas, int x = 0, int y = 0) const;
    // The same with each pixel's colour changed through `colours`.
    void draw(std::size_t index, Canvas& canvas, int x, int y, const ColourMap& colours) const;

private:
    int width_ = 0;
    int height_ = 0;
    std::uint16_t flags_ = 0;
    std::uint16_t loopFrame_ = 0;
    std::optional<PaletteChange> palette_;
    std::vector<std::vector<std::uint8_t>> frames_;
};

} // namespace hegemon
