// SPDX-License-Identifier: GPL-3.0-or-later
//
// The colony screen: the planet's landscape under its sky, the panels at
// the top and the buttons, drawn as the original draws them. See
// docs/rules/colony-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/ui/pit_timer.hpp"
#include "hegemon/ui/product_picture.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <utility>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class GameData;
class Economy;

class ColonyScreen {
public:
    // The pointers in this screen's colours: entry 2 of FONTS.LBX.
    static constexpr std::size_t kPointerEntry = 2;
    // Measured: the Return button at the bottom right.
    static constexpr Rect kReturnButton = {552, 457, 77, 22};
    // Measured: Change, which opens the build list.
    static constexpr Rect kChangeButton = {520, 124, 60, 20};
    // Measured: Buy, beside it.
    static constexpr Rect kBuyButton = {590, 124, 37, 20};

    // Reads the pictures for a colony of the game. Throws FormatError when a
    // file is damaged.
    ColonyScreen(GameData& data, const Game& game, const Economy& economy, std::size_t colony);

    const std::string& title() const { return title_; }
    const std::string& population() const { return population_; }
    // The box of what the colony builds: its name (empty with nothing to
    // build), the turns it takes and how far it is.
    const std::string& productText() const { return product_; }
    const std::string& turnsText() const { return turns_; }
    const std::string& progressText() const { return progress_; }

    // What Buy says: the question for an offer, the message otherwise;
    // empty for an undo, which says nothing (assumed).
    std::string buyText(const Purchase& purchase) const;

    // The ways a home colony's landscape is laid out, as measured: which
    // building or decoration stands on which of the 36 places, and the roads.
    static constexpr int kLayouts = 4;

    // The rows of icons at the top: money, food, production and research.
    // Each holds COLONY2.LBX entries from the left, kGap for an empty place.
    static constexpr int kGap = -1;
    enum Row : std::size_t { Money, Food, Production, Research, kRows };
    const std::vector<int>& row(Row r) const { return rows_.at(r); }
    // The planets of the colony's system, by orbit: their texts, one or two
    // lines, empty for an empty orbit.
    const std::array<std::string, 2>& orbitText(std::size_t orbit) const { return orbits_.at(orbit).lines; }

    // How far apart a row's icons are drawn.
    static int iconStep(std::size_t places);

    // The mouse: what a button let go over does.
    // Jobs: people were moved to another job; jobs() tells the new numbers.
    enum class Action : std::uint8_t { None, Return, Change, Buy, Jobs };
    void press(int x, int y);
    Action release(int x, int y);
    void pointerMoved(int x, int y);

    // The people in each job (farmers, workers, scientists), after a move.
    const std::array<int, 3>& jobs() const { return jobs_; }
    // Where a person of a job row is, (x, y) its top left; -1 when the place
    // is not on a person.
    int personAt(int x, int y, std::size_t& job) const;
    // The people held, being moved: their job and how many, 0 for none.
    std::size_t heldJob() const { return heldJob_; }
    int heldCount() const { return heldCount_; }

    // Hegemon's keyboard and gamepad: choose a job row, then pick up its
    // last person, choose the row to move it to and put it down there.
    std::size_t cursorRow() const { return cursor_; }
    void moveCursor(int step);
    Action pickOrDrop();
    // Where the pointer goes for the chosen row.
    std::pair<int, int> cursorPoint() const;

    // The sky, the landscape and its buildings, without the panels: the
    // build list shows it behind its own.
    const Canvas& scene() const { return scene_; }
    // The colours the scene is drawn in.
    Palette palette() const;

    // Moves the arrow by the colony shown on, by `ms` milliseconds.
    void tick(std::uint64_t ms);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    struct Orbit {
        bool planet = false;
        bool shown = false; // the colony shown
        Image icon;
        std::array<std::string, 2> lines;
    };
    // Draws a building or road, darkening what its shadow falls on.
    void drawShaded(const Image& picture, Canvas& canvas) const;
    void planetList(GameData& data, const Game& game, const Economy& economy, const Colony& shown,
                    const std::vector<std::string>& messages);

    Palette base_;
    Image sky_;
    Image landscape_;
    Image frame_;
    std::vector<Image> icons_; // COLONY2.LBX entries 0 to 26
    std::array<Image, 3> people_; // a farmer, a worker and a scientist
    std::array<std::vector<int>, kRows> rows_;
    std::array<int, 3> jobs_{};
    int government_ = 0;
    Image arrow_;
    struct Piece {
        int place = 0;
        Image picture;
    };
    std::vector<Image> roads_;
    std::vector<Piece> pieces_; // drawn from the back
    std::array<std::uint8_t, 256> shade_{}; // each colour in a shadow
    Canvas scene_{640, 480}; // the sky, the landscape, the roads and buildings
    std::size_t arrowFrame_ = 0;
    PitTimer timer_;
    std::array<Orbit, kOrbits> orbits_;
    FontSet fonts_;
    std::string title_;
    std::string population_;
    std::string product_, turns_, progress_;
    std::optional<ProductPicture> picture_; // of what is built
    std::string buyQuestion_, buyShort_, buyNotBuyable_, buyThisTurn_, buyNothing_;
    Rect held_{};
    // People being moved, with the mouse or the keyboard.
    std::size_t heldJob_ = 0;
    int heldCount_ = 0;
    bool keyboardHold_ = false;
    int pointerX_ = 0, pointerY_ = 0;
    std::size_t cursor_ = 0;
    std::size_t rowAt(int y, bool& inside) const;
    Action drop(std::size_t row);
};

} // namespace hegemon
