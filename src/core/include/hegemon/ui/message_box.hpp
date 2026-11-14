// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's two small boxes over a screen: a message with Close
// (TEXTBOX.LBX, green text) and a question with Yes and No (CONFIRM.LBX,
// orange text), as measured. See docs/rules/colony-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/ui/rect.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class MessageBox {
public:
    enum class Kind : std::uint8_t { Message, Question };
    enum class Answer : std::uint8_t { None, Close, Yes, No };

    // Breaks `text` into the box's lines. Throws FormatError when a file is
    // damaged.
    MessageBox(GameData& data, Kind kind, const std::string& text);

    Kind kind() const { return kind_; }
    const std::vector<std::string>& lines() const { return lines_; }
    // Where the buttons are: Close for a message, Yes and No for a question.
    Rect closeRect() const;
    static constexpr Rect kYes = {235, 302, 54, 24};
    static constexpr Rect kNo = {345, 302, 55, 24};

    // The mouse: an answer when a button is let go over.
    void press(int x, int y);
    Answer release(int x, int y);

    // Draws the box over what is on the canvas, in the colours of `palette`
    // nearest to its own.
    void draw(Canvas& canvas, const Palette& palette) const;

private:
    int top() const;
    int bodyHeight() const;

    Kind kind_;
    std::vector<Image> pictures_;
    FontSet fonts_;
    std::vector<std::string> lines_;
    Rect held_{};
};

} // namespace hegemon
