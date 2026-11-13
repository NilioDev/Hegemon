// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Select Banner Color screen that follows the ruler's name, and the
// "Generating Universe ..." screen after it, drawn and laid out as the
// original draws them. See docs/rules/banner.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class BannerSelectScreen {
public:
    // The eight colours in the original's order: the top row, then the
    // bottom row.
    enum class Colour : std::size_t { Red, Yellow, Green, Silver, Blue, Brown, Purple, Orange };
    static constexpr std::size_t kBanners = 8;
    // The pointers in the race selection's colours: entry 10 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 10;

    // Reads the screen's pictures with the banners of `race` (0 to 12).
    // Throws FormatError when a file is damaged.
    BannerSelectScreen(GameData& data, std::size_t race);

    Rect bannerRect(std::size_t banner) const;
    std::optional<std::size_t> bannerAt(int x, int y) const;

    // The mouse: a banner is taken when the mouse button is let go over it.
    void press(int x, int y);
    std::optional<Colour> release(int x, int y);
    std::optional<Colour> click(int x, int y);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    Image background_;
    Image panel_;
    std::vector<Image> banners_;
    bool held_ = false;
};

// What the screen shows while the galaxy is made.
class GeneratingScreen {
public:
    // Throws FormatError when a file is damaged.
    explicit GeneratingScreen(GameData& data);

    const std::string& text() const { return text_; }

    // Draws over the colours the banner screen left.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    Image background_;
    FontSet fonts_;
    std::string text_;
};

} // namespace hegemon
