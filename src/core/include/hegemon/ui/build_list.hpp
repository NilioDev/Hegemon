// SPDX-License-Identifier: GPL-3.0-or-later
//
// The colony's build list, opened with Change on the colony screen: what it
// can build on the left and right, its build list in the middle, and the
// cost of what the pointer is on. See docs/rules/colony-screen.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/ui/product_picture.hpp"
#include "hegemon/ui/rect.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

class GameData;
class Economy;

// The name the original gives a product (rules/production.hpp, or a
// building of the fan patch's table); a design's comes from its `owner`.
// Throws FormatError when a file is damaged.
std::string productName(GameData& data, std::string_view product, const Player* owner = nullptr);
// A design's name: its own, the race's name for it from SHIPNAME.LBX, or
// the Scout's.
std::string designName(GameData& data, int race, const ShipDesign& design);

class BuildListScreen {
public:
    // Measured on the original.
    static constexpr Rect kOkButton = {560, 447, 64, 19};
    static constexpr Rect kCancelButton = {493, 447, 62, 19};
    // Rows of the lists: on the left from the top, on the right ships and
    // spies at their places.
    static constexpr std::size_t kRightRows = 14;

    // The colony's scene (ColonyScreen::scene()) in the colours `colours`
    // is shown behind it. Throws FormatError when a file is damaged.
    BuildListScreen(GameData& data, const Game& game, const Economy& economy, std::size_t colony, const Canvas& scene,
                    const Palette& colours);

    enum class Column : std::uint8_t { Left, Right, Queue };
    struct Slot {
        Column column = Column::Left;
        std::size_t row = 0;
        bool operator==(const Slot&) const = default;
    };

    // The products shown: on the left from the top; on the right by row,
    // empty where there is none.
    const std::vector<std::string>& left() const { return left_; }
    const std::array<std::string, kRightRows>& right() const { return right_; }
    const std::vector<std::string>& buildList() const { return colony_.buildList; }
    // The name the original gives a product.
    const std::string& name(const std::string& product) const;

    // What the pointer is on, as the box at the top tells it: the name, then
    // "Cost Here" and "Build Time" (empty for housing and trade goods).
    std::optional<Slot> hovered() const { return hovered_; }
    const std::string& hoverName() const { return hoverName_; }
    const std::string& costText() const { return costText_; }
    const std::string& timeText() const { return timeText_; }
    // "Maint. Cost" for a building or colony base, and the description
    // below, broken into its lines (empty for housing and trade goods).
    const std::string& maintenanceText() const { return maintenanceText_; }
    const std::vector<std::vector<std::string>>& description() const { return description_; }
    // For a design, the box below lists its parts instead: texts and where
    // they go.
    struct PlacedText {
        int x = 0, y = 0;
        std::string text;
    };
    const std::vector<PlacedText>& designText() const { return designText_; }

    // The mouse.
    void hover(int x, int y);
    enum class Result : std::uint8_t { None, Ok, Cancel };
    void press(int x, int y);
    Result release(int x, int y);
    // The keyboard and gamepad: moves the highlight a row (dy) or a column
    // (dx), and adds or deletes what it is on.
    void moveHighlight(int dx, int dy);
    void choose();

    void draw(Canvas& canvas, Palette& palette) const;

private:
    std::optional<Slot> slotAt(int x, int y) const;
    const std::string* productAt(const Slot& slot) const;
    void act(const Slot& slot);
    void setHovered(std::optional<Slot> slot);
    std::size_t rows(Column column) const;

    Colony colony_; // a copy whose list is changed until OK
    Palette base_;
    Image frame_;
    std::array<Image, 2> buttons_; // Cancel, OK
    Image refit_, design_;
    Canvas background_{640, 480};
    FontSet fonts_;
    std::vector<std::string> left_;
    std::array<std::string, kRightRows> right_;
    std::vector<std::pair<std::string, std::string>> names_; // product, name
    std::vector<std::pair<std::string, int>> costs_;
    std::vector<std::pair<std::string, ProductPicture>> pictures_;
    std::vector<std::pair<std::string, std::string>> descriptions_; // product, text
    std::vector<std::pair<std::string, int>> maintenance_;          // buildings and the colony base
    std::vector<std::pair<std::string, std::vector<PlacedText>>> designTexts_; // designs, by product
    std::vector<PlacedText> designText_;
    std::string title_;
    std::string deleteFormat_, costFormat_, timeFormat_, maintenanceFormat_;
    int production_ = 0;
    std::optional<Slot> hovered_;
    std::string hoverName_, costText_, timeText_, maintenanceText_;
    std::vector<std::vector<std::string>> description_; // lines of words
    std::optional<Slot> held_;
    int heldButton_ = -1; // 0 Cancel, 1 OK
    // Colours of the text (font_color_group g0): for a product not in the
    // build list and for one in it.
    std::array<std::uint8_t, 3> normal_{}, queued_{};
};

} // namespace hegemon
