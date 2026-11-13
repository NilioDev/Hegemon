// SPDX-License-Identifier: GPL-3.0-or-later
//
// A rectangle on the 640×480 screen.
#pragma once

namespace hegemon {

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }
};

} // namespace hegemon
