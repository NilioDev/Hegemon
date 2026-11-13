// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's bitmap fonts (entry 0 of FONTS.LBX and IFONTS.LBX) and
// drawing text with them. See docs/formats/fonts.md.
#pragma once

#include "hegemon/data/image.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

// Where a shadow pixel goes, relative to a drawn pixel of a glyph.
struct ShadowOffset {
    int dx, dy;
};

// What colour each shade of a glyph is drawn in (shade 0 is never drawn,
// nor a shade without a colour),
// and the colour of the shadow, if there is one. The shadow is drawn first,
// at each of `shadowAt` from every drawn pixel; the glyphs go over it.
struct TextStyle {
    std::array<std::optional<std::uint8_t>, 16> shades{};
    std::optional<std::uint8_t> shadow;
    std::vector<ShadowOffset> shadowAt{{1, 1}};
    int spacing = 1; // pixels between two characters
};

// The part of the canvas text may change: left and top included, right and
// bottom not.
struct TextClip {
    int left = std::numeric_limits<int>::min();
    int top = std::numeric_limits<int>::min();
    int right = std::numeric_limits<int>::max();
    int bottom = std::numeric_limits<int>::max();
    bool contains(int x, int y) const { return x >= left && y >= top && x < right && y < bottom; }
};

class Font {
public:
    int height() const { return height_; }
    int width(unsigned char c) const { return widths_[c]; }
    // Width of a line of text: the characters and the spacing after each.
    int textWidth(std::string_view text, int spacing) const;
    // Draws `text` with its top left corner at (x, y), leaving every pixel
    // outside `clip` alone, and returns the x after the last character and
    // its spacing.
    int draw(Canvas& canvas, int x, int y, std::string_view text, const TextStyle& style,
             const TextClip& clip = {}) const;

private:
    friend class FontSet;
    struct Glyph {
        // Rows from the top; each pixel is a shade, 0 for nothing.
        std::vector<std::vector<std::uint8_t>> rows;
    };
    int height_ = 0;
    std::array<std::uint8_t, 256> widths_{};
    std::array<Glyph, 256> glyphs_;
};

class FontSet {
public:
    static constexpr std::size_t kFonts = 6;

    // Checks every table and glyph against the size of the entry; throws
    // FormatError when anything is out of place.
    static FontSet parse(std::span<const std::uint8_t> entry);

    const Font& font(std::size_t index) const { return fonts_.at(index); }

private:
    std::array<Font, kFonts> fonts_;
};

// Breaks a text into lines at its spaces, putting as many words on each
// line as fit in `maxWidth` pixels (up to the last pixel of the last
// letter). A word wider than that gets a line of its own.
std::vector<std::string> wrapWords(const Font& font, std::string_view text, int maxWidth, int spacing);

} // namespace hegemon
