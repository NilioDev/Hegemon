// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Enter Ruler Name dialog that follows the race selection, drawn and
// laid out as the original draws it. See docs/rules/ruler-name.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/name_box.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

// The names the original offers a stock race's ruler: entry 0 of
// RACENAME.LBX, eight for each race. See docs/formats/races.md.
class RulerNames {
public:
    static constexpr std::size_t kPerRace = 8;

    // Throws FormatError when the table is damaged or too short.
    explicit RulerNames(GameData& data);

    const std::string& name(std::size_t race, std::size_t choice) const;

private:
    std::vector<std::string> names_;
};

class RulerNameDialog {
public:
    static constexpr std::size_t kMaxLength = NameBox::kMaxLength;
    static constexpr int kMaxWidth = NameBox::kMaxWidth;
    // The pointers in the race selection's colours: entry 10 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 10;

    // Reads the dialog's pictures and title. `race` is the button taken in
    // the race selection (0 to 13), whose portrait's colours the screen
    // keeps; `name` is the one offered. Throws FormatError when a file is
    // damaged.
    RulerNameDialog(GameData& data, std::size_t race, std::string name);

    const std::string& name() const { return box_.name(); }

    bool type(char c) { return box_.type(c); }
    void backspace() { box_.backspace(); }

    Rect acceptRect() const { return box_.acceptRect(); }
    void press(int x, int y) { box_.press(x, y); }
    bool release(int x, int y) { return box_.release(x, y); }
    bool click(int x, int y) { return box_.click(x, y); }

    void tick(std::uint64_t ms) { box_.tick(ms); }
    bool cursorShown() const { return box_.cursorShown(); }

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    static std::string title(GameData& data);

    Image background_;
    Palette palette_; // the screen's colours
    NameBox box_;
};

} // namespace hegemon
