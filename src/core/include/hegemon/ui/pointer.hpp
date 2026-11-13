// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's mouse pointer, shared by all screens: it stays where it is
// when the screen changes. Each screen draws it from the entry of FONTS.LBX
// that suits its colours. See docs/formats/fonts.md.
#pragma once

#include "hegemon/data/image.hpp"
#include "hegemon/data/pointers.hpp"

#include <map>

namespace hegemon {

class GameData;

class Pointer {
public:
    // The main menu's pointers, entry 6 of FONTS.LBX.
    static constexpr std::size_t kMainMenuEntry = 6;

    // Reads the main menu's pointers from FONTS.LBX. Throws FormatError when
    // the entry is damaged.
    explicit Pointer(GameData& data);

    // Draws from now on with the pointers of `entry` (1 to 13) of FONTS.LBX.
    // Throws FormatError when it is damaged.
    void useEntry(std::size_t entry);

    // The mouse moved to (x, y) on the 640×480 screen; this also shows the
    // pointer again.
    void move(int x, int y);
    // Hides the pointer while the gamepad or keyboard is used (Hegemon's
    // addition), and when the mouse leaves the window.
    void hide() { visible_ = false; }
    void show() { visible_ = true; }
    bool visible() const { return visible_; }
    int x() const { return x_; }
    int y() const { return y_; }

    // Draws the pointer with its hot spot, the top left corner, at (x, y).
    void draw(Canvas& canvas) const;

private:
    GameData* data_;
    std::map<std::size_t, PointerSet> sets_;
    const PointerSet* pointers_ = nullptr;
    int x_ = 320, y_ = 240; // the original starts it mid-screen
    bool visible_ = true;
};

} // namespace hegemon
