// SPDX-License-Identifier: GPL-3.0-or-later
//
// The main menu, drawn and laid out as the original draws it. See
// docs/rules/main-menu.md.
#pragma once

#include "hegemon/data/image.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/ui/credits.hpp"

#include <array>
#include <optional>
#include <vector>

namespace hegemon {

class MainMenu {
public:
    enum class Choice { Continue, Load, NewGame, Multiplayer, HallOfFame, Quit };
    static constexpr std::size_t kButtons = 6;

    // Reads the menu's pictures and finds out from the savegames which
    // buttons can be used. Throws FormatError when a file is damaged.
    explicit MainMenu(GameData& data);

    bool enabled(Choice choice) const { return enabled_[index(choice)]; }
    std::optional<Choice> highlighted() const { return highlighted_; }

    // The mouse moved to (x, y) on the 640×480 screen.
    void pointerMoved(int x, int y);
    // A click at (x, y): the choice of the enabled button there, if any.
    std::optional<Choice> click(int x, int y);

    // Gamepad and keyboard: move the highlight to the next or previous button
    // that can be used, and take the highlighted one.
    void moveHighlight(int step);
    std::optional<Choice> activate() const;

    // Lets `ms` milliseconds pass, for the scrolling credits.
    void tick(std::uint64_t ms) { credits_.tick(ms); }
    const Credits& credits() const { return credits_; }
    Credits& credits() { return credits_; }

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

    // Where button `i` sits on the screen, from the top.
    struct Rect {
        int x, y, w, h;
        bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    };
    Rect buttonRect(std::size_t i) const;

private:
    static std::size_t index(Choice c) { return static_cast<std::size_t>(c); }
    std::optional<std::size_t> buttonAt(int x, int y) const;

    Image background_;
    Image panel_;
    Credits credits_;
    // Per button: normal, highlighted and greyed out.
    std::vector<std::array<Image, 3>> buttons_;
    std::array<bool, kButtons> enabled_{};
    std::optional<Choice> highlighted_;
};

} // namespace hegemon
