// SPDX-License-Identifier: GPL-3.0-or-later
//
// The main menu, drawn and laid out as the original draws it. See
// docs/rules/main-menu.md.
#pragma once

#include "hegemon/data/image.hpp"
#include "hegemon/data/pointers.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/ui/credits.hpp"
#include "hegemon/ui/pit_timer.hpp"

#include <array>
#include <optional>
#include <string>
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

    // Ticks of the PC timer from the start of the opening animation (one
    // frame a tick) until the buttons appear, and until the credits start.
    static constexpr std::uint64_t kOpenedTick = 50;
    static constexpr std::uint64_t kCreditsStartTick = 28;

    // The mouse moved to (x, y) on the 640×480 screen. This also shows the
    // original's mouse pointer there.
    void pointerMoved(int x, int y);
    // Hides the pointer while the gamepad or keyboard is used (Hegemon's
    // addition); the next mouse move shows it again.
    void hidePointer() { pointerVisible_ = false; }
    bool pointerVisible() const { return pointerVisible_; }
    // A click at (x, y): the choice of the enabled button there, if any.
    std::optional<Choice> click(int x, int y);

    // Gamepad and keyboard: move the highlight to the next or previous button
    // that can be used, and take the highlighted one.
    void moveHighlight(int step);
    std::optional<Choice> activate();

    // Lets `ms` milliseconds pass: the opening animation and the credits.
    void tick(std::uint64_t ms);

    // The menu opens with an animation; the buttons appear after it. Until
    // then the pointer, clicks and the gamepad do nothing, and any key, click
    // or button skips to the end.
    bool opening() const { return openingTicks_ < kOpenedTick; }
    void skipOpening();
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

    // A text on the screen, placed by the fan patch's `format` setting.
    struct Label {
        std::string text;
        std::size_t font = 0;
        char align = 'l'; // l, r or c
        int x = 0, y = 0;
    };
    const Label& versionLabel() const { return version_; }
    const Label& modNameLabel() const { return modName_; }

private:
    static std::size_t index(Choice c) { return static_cast<std::size_t>(c); }
    std::optional<std::size_t> buttonAt(int x, int y) const;
    void drawLabel(Canvas& canvas, const Label& label) const;
    void drawPointer(Canvas& canvas) const;

    void advanceOpening(std::uint64_t ticks);

    Image background_;
    Image panel_;
    Image opening_;
    PointerSet pointers_;
    int pointerX_ = 320, pointerY_ = 240; // the original starts it mid-screen
    bool pointerVisible_ = true;
    Canvas opened_{640, 480};
    std::size_t openingFrames_ = 0; // frames drawn into opened_
    std::uint64_t openingTicks_ = 0;
    PitTimer timer_;
    Credits credits_;
    // Per button: normal, highlighted and greyed out.
    std::vector<std::array<Image, 3>> buttons_;
    std::array<bool, kButtons> enabled_{};
    std::optional<Choice> highlighted_;
    Label version_;
    Label modName_;
};

} // namespace hegemon
