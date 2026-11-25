// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Colonies screen (COLSUM.LBX) that the galaxy map's COLONIES button
// opens: a row for each of the player's colonies, what the colony under the
// pointer makes, a small map of the galaxy and the empire's totals. See
// docs/rules/colonies-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/colony_screen.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hegemon {

class GameData;
class Economy;

class ColoniesScreen {
public:
    // The pointers in this screen's colours, as in the colony screen.
    static constexpr std::size_t kPointerEntry = ColonyScreen::kPointerEntry;
    // Ten rows show at a time.
    static constexpr std::size_t kRowsShown = 10;
    // The buttons along the bottom, in this order, sort the rows.
    enum class Sort : std::uint8_t { Name, Population, Food, Industry, Science, Producing, Money };
    static constexpr std::size_t kSorts = 7;

    // Measured: Return, and the arrows above and below the rows.
    static constexpr Rect kReturnButton = {531, 445, 85, 26};
    static constexpr Rect kUpButton = {619, 15, 10, 20};
    static constexpr Rect kDownButton = {619, 316, 11, 22};
    static Rect sortRect(Sort sort);
    // The parts of a row shown: its colony's name, the farmers, workers and
    // scientists, what it builds and the button that buys it.
    enum class Column : std::uint8_t { Name, Farmers, Workers, Scientists, Building, Buy };
    static constexpr std::size_t kColumns = 6;
    static Rect cellRect(std::size_t row, Column column);

    // The colonies of `player`. Throws FormatError when a file is damaged.
    ColoniesScreen(GameData& data, const Game& game, const Economy& economy, int player);

    // The colonies (indices into Game::colonies) in the order shown, and
    // the first one in the top row.
    const std::vector<std::size_t>& order() const { return colonies_; }
    std::size_t top() const { return top_; }
    Sort sort() const { return sort_; }
    void setSort(Sort sort);
    // Moves the rows by `rows`, as far as there are colonies.
    void scroll(int rows);
    // The colony whose details show at the bottom: the one last under the
    // pointer, at first the top one; none without colonies.
    std::optional<std::size_t> shown() const;

    // What a row says.
    const std::string& name(std::size_t colony) const;
    const std::string& building(std::size_t colony) const;
    // The lines at the bottom left about the colony shown.
    const std::vector<std::string>& details() const;
    // The empire's totals at the bottom right: "Reserve:" and "58", ...
    const std::vector<std::pair<std::string, std::string>>& totals() const { return totals_; }

    // The mouse. Open: show the colony screen of `colony`; Build: its build
    // list; Buy: buy what it builds; Jobs: people were moved, jobs() says
    // how many now do each job.
    struct Action {
        enum class Kind : std::uint8_t { None, Return, Open, Build, Buy, Jobs };
        Kind kind = Kind::None;
        std::size_t colony = 0;
    };
    void press(int x, int y);
    Action release(int x, int y);
    void pointerMoved(int x, int y);
    const std::array<int, kJobs>& jobs(std::size_t colony) const;
    // The people held: how many, 0 for none.
    int heldCount() const { return heldCount_; }

    // Hegemon's keyboard and gamepad: where the pointer goes from (x, y)
    // when stepped by (dx, dy) over the rows' parts and the buttons below.
    std::pair<int, int> step(int x, int y, int dx, int dy) const;

    void draw(Canvas& canvas, Palette& palette) const;

private:
    struct Row {
        std::size_t colony = 0;
        std::string name;
        std::array<int, kJobs> jobs{};
        int population = 0, food = 0, industry = 0, science = 0, money = 0;
        std::string product;
        std::string building;
        std::array<std::vector<int>, ColonyScreen::kRows> icons;
        std::vector<std::string> details;
        int star = 0;
    };
    const Row* rowAt(std::size_t visible) const;
    std::optional<std::size_t> visibleRowAt(int y) const;
    void showRow(std::size_t visible);

    Image background_, return_, up_, down_;
    std::array<Image, kSorts> sorts_;
    std::array<Image, kRowsShown> buyButtons_;
    std::vector<Image> icons_; // COLONY2.LBX's smaller icons
    std::array<Image, kJobs> people_;
    FontSet fonts_;
    std::vector<Row> rows_;
    std::vector<std::size_t> order_; // into rows_
    std::vector<std::size_t> colonies_; // the same, as colony indices
    struct Dot {
        int x = 0, y = 0;
        bool blackHole = false;
    };
    std::vector<Dot> stars_;
    std::vector<std::string> starNames_;
    std::vector<std::pair<std::string, std::string>> totals_;
    std::size_t top_ = 0;
    std::size_t shown_ = 0; // into rows_
    Sort sort_ = Sort::Name;
    Rect held_{};
    std::size_t heldRow_ = 0; // into rows_
    std::size_t heldJob_ = 0;
    int heldCount_ = 0;
    int pointerX_ = 0, pointerY_ = 0;

    void sortRows();
};

} // namespace hegemon
