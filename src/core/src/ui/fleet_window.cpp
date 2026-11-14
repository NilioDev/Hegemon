// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/fleet_window.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/fleets.md), all BUFFER0.LBX: the
// title piece, the ship boxes (three rows of three, cut to the rows the
// fleet needs), the piece with All and the status, and the buttons.
constexpr const char* kArchive = "BUFFER0.LBX";
constexpr std::size_t kTopEntry = 52, kGridEntry = 54, kFootEntry = 56, kChosenEntry = 11, kAllEntry = 60,
                      kCloseEntry = 72, kColonizeEntry = 69;
constexpr int kLeft = 22, kTop = 22;
constexpr int kGridTop = 57, kRowHeight = 56, kMaxRows = 3, kColumns = 3;
// A ship's box: its top left corner, the step to the next, its size; the
// ship's picture 1 right of and 2 below the box's corner.
constexpr int kBoxX = 37, kBoxY = 60, kBoxStepX = 58, kBoxW = 54, kBoxH = 53;
constexpr int kPictureDx = 1, kPictureDy = 2;
// Below the boxes: All 4 lower and 17 right, the status 10 lower, centred
// on x 150; Close under the piece (it is 37 high).
constexpr int kAllDx = 17, kAllDy = 4, kStatusDy = 10, kStatusCentre = 150;
constexpr int kFootHeight = 37, kButtonX = 24;
// Texts: the title in font 4, the status in font 3, in greens.
constexpr std::size_t kTitleFont = 4, kStatusFont = 3;
constexpr int kTitleY = 36, kTitleSpacing = 1, kStatusSpacing = 2;
constexpr int kTitleCentre = kLeft + 99;
constexpr std::uint8_t kTitleShade1 = 111, kTitleShade2 = 115, kTitleShade3 = 113;
constexpr std::uint8_t kStatusShade1 = 112, kStatusShade2 = 115;
// The title has the map's dark outline: drawn around itself in this order,
// shade 1 in colour 1, the others in colour 2 (measured).
constexpr std::uint8_t kOutline1 = 1, kOutline2 = 2;
const std::vector<ShadowOffset> kOutlineOrder = {{-1, 0}, {1, 1}, {-1, -1}, {0, -1}, {1, 0}, {0, 1}, {1, -1}, {-1, 1}};
// HESTRNGS.LBX: "%s Fleet", "Orbiting %s", "ETA %d turn(s)"; ESTRINGS.LBX
// from 645: each race's name.
constexpr std::size_t kFleetTitle = 102, kOrbiting = 103, kEtaTurn = 107, kEtaTurns = 108;
constexpr std::size_t kRaceNames = 645;
// SHIPS.LBX: fifty entries for each banner colour, the ships' pictures and
// last their colours (192 on); the landing ships' pictures.
constexpr std::size_t kShipsPerBanner = 50, kShipColours = 49;
constexpr int kColonyShipPicture = 45, kOutpostShipPicture = 46, kTransportPicture = 47;

std::string fill(const std::string& message, const std::string& text) {
    std::string s = message;
    const auto at = s.find("%s");
    if (at != std::string::npos) {
        s.replace(at, 2, text);
    }
    return s;
}

std::string fill(const std::string& message, int n) {
    std::string s = message;
    const auto at = s.find("%d");
    if (at != std::string::npos) {
        s.replace(at, 2, std::to_string(n));
    }
    return s;
}

int pictureOf(const Game& game, const Ship& ship) {
    const auto& designs = game.players.at(static_cast<std::size_t>(ship.player)).designs;
    switch (ship.kind) {
    case ShipKind::ColonyShip:
        return kColonyShipPicture;
    case ShipKind::OutpostShip:
        return kOutpostShipPicture;
    case ShipKind::Transport:
        return kTransportPicture;
    case ShipKind::Combat:
        if (ship.design >= 0 && static_cast<std::size_t>(ship.design) < designs.size()) {
            return designs[static_cast<std::size_t>(ship.design)].image;
        }
        return 0;
    case ShipKind::Scout:
        break;
    }
    return designs.empty() ? 0 : designs.front().image;
}

} // namespace

FleetWindow::FleetWindow(GameData& data, const Game& game, const Fleet& fleet, std::size_t banner)
    : top_(Image::parse(data.archive(kArchive).entry(kTopEntry))),
      grid_(Image::parse(data.archive(kArchive).entry(kGridEntry))),
      foot_(Image::parse(data.archive(kArchive).entry(kFootEntry))),
      box_(Image::parse(data.archive(kArchive).entry(kChosenEntry))),
      all_(Image::parse(data.archive(kArchive).entry(kAllEntry))),
      close_(Image::parse(data.archive(kArchive).entry(kCloseEntry))),
      colonize_(Image::parse(data.archive(kArchive).entry(kColonizeEntry))),
      shipColours_(Image::parse(data.archive("SHIPS.LBX").entry(std::min<std::size_t>(banner, 7) * kShipsPerBanner + kShipColours))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    strings_ = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings_.size() <= kEtaTurns) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    const auto races = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    const Player& owner = game.players.at(static_cast<std::size_t>(fleet.player));
    const auto race = kRaceNames + static_cast<std::size_t>(std::max(owner.race, 0));
    if (races.size() <= race) {
        throw FormatError("too few messages in ESTRINGS.LBX");
    }
    title_ = fill(strings_[kFleetTitle], races[race]);
    const auto& ships = data.archive("SHIPS.LBX");
    for (const std::size_t i : fleet.ships) {
        const auto picture = static_cast<std::size_t>(std::clamp(pictureOf(game, game.ships.at(i)), 0,
                                                                 static_cast<int>(kShipColours) - 1));
        pictures_.push_back(Image::parse(ships.entry(std::min<std::size_t>(banner, 7) * kShipsPerBanner + picture)));
    }
    ships_ = fleet.ships;
    chosen_.assign(ships_.size(), true);
    describe(game, fleet);
}

void FleetWindow::describe(const Game& game, const Fleet& fleet) {
    canColonize_ = false;
    if (fleet.destination < 0 && fleet.star >= 0 && !colonySites(game, fleet.star).empty()) {
        for (const std::size_t i : fleet.ships) {
            if (game.ships.at(i).kind == ShipKind::ColonyShip) {
                canColonize_ = true;
            }
        }
    }
    if (fleet.destination >= 0 && !fleet.ships.empty()) {
        const Star& to = game.galaxy.stars.at(static_cast<std::size_t>(fleet.destination));
        const int speed = std::max(1, game.ships.at(fleet.ships.front()).speed);
        const int turns = travelTurns(fleet.place, {to.x, to.y}, speed);
        status_ = fill(strings_[turns == 1 ? kEtaTurn : kEtaTurns], turns);
    } else if (fleet.star >= 0) {
        status_ = fill(strings_[kOrbiting], game.galaxy.stars.at(static_cast<std::size_t>(fleet.star)).name);
    } else {
        status_.clear();
    }
}

void FleetWindow::update(const Game& game, const Fleet& fleet) {
    std::vector<Image> pictures;
    for (const std::size_t i : fleet.ships) {
        const auto at = std::find(ships_.begin(), ships_.end(), i);
        if (at != ships_.end()) {
            pictures.push_back(pictures_[static_cast<std::size_t>(at - ships_.begin())]);
        }
    }
    if (pictures.size() == fleet.ships.size()) {
        pictures_ = std::move(pictures);
        ships_ = fleet.ships;
    }
    // Measured: the boxes are no longer marked once the fleet is sent off.
    chosen_.assign(ships_.size(), fleet.destination < 0);
    describe(game, fleet);
}

std::vector<std::size_t> FleetWindow::chosen() const {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < ships_.size(); ++i) {
        if (chosen_[i]) {
            result.push_back(ships_[i]);
        }
    }
    return result;
}

int FleetWindow::rows() const {
    const int n = static_cast<int>(ships_.size());
    return std::clamp((n + kColumns - 1) / kColumns, 1, kMaxRows);
}

int FleetWindow::bottom() const { return kGridTop + rows() * kRowHeight; }

Rect FleetWindow::area() const {
    const int buttons = close_.height() + (canColonize_ ? colonize_.height() : 0);
    return {kLeft, kTop, top_.width(), bottom() + kFootHeight + buttons - kTop};
}

Rect FleetWindow::shipBox(std::size_t index) const {
    const int i = static_cast<int>(index);
    return {kBoxX + (i % kColumns) * kBoxStepX, kBoxY + (i / kColumns) * kRowHeight, kBoxW, kBoxH};
}

Rect FleetWindow::allButton() const { return {kLeft + kAllDx, bottom() + kAllDy, all_.width(), all_.height()}; }

Rect FleetWindow::closeButton() const { return {kButtonX, bottom() + kFootHeight, close_.width(), close_.height()}; }

std::optional<Rect> FleetWindow::colonizeButton() const {
    if (!canColonize_) {
        return std::nullopt;
    }
    const Rect close = closeButton();
    return Rect{kButtonX, close.y + close.h, colonize_.width(), colonize_.height()};
}

void FleetWindow::press(int x, int y) {
    held_ = {};
    for (const Rect r : {allButton(), closeButton(), colonizeButton().value_or(Rect{})}) {
        if (r.contains(x, y)) {
            held_ = r;
        }
    }
    for (std::size_t i = 0; i < ships_.size() && i < static_cast<std::size_t>(kColumns * kMaxRows); ++i) {
        if (shipBox(i).contains(x, y)) {
            held_ = shipBox(i);
        }
    }
}

FleetWindow::Action FleetWindow::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    if (!held.contains(x, y)) {
        return Action::None;
    }
    if (closeButton().contains(x, y)) {
        return Action::Close;
    }
    if (const auto colonize = colonizeButton(); colonize && colonize->contains(x, y)) {
        return Action::Colonize;
    }
    if (allButton().contains(x, y)) {
        chosen_.assign(ships_.size(), true);
        return Action::None;
    }
    for (std::size_t i = 0; i < ships_.size(); ++i) {
        if (shipBox(i).contains(x, y)) {
            chosen_[i] = !chosen_[i];
        }
    }
    return Action::None;
}

void FleetWindow::draw(Canvas& canvas, Palette& palette) const {
    shipColours_.applyPalette(palette);
    top_.draw(0, canvas, kLeft, kTop);
    // The boxes, cut to the rows used.
    Canvas grid(grid_.width(), grid_.height(), 0);
    grid_.draw(0, grid);
    for (int y = 0; y < rows() * kRowHeight && y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const auto v = grid.at(x, y);
            const int cx = kLeft + x, cy = kGridTop + y;
            if (v != 0 && cx < canvas.width() && cy < canvas.height()) {
                canvas.set(cx, cy, v);
            }
        }
    }
    for (std::size_t i = 0; i < ships_.size() && i < static_cast<std::size_t>(kColumns * kMaxRows); ++i) {
        const Rect r = shipBox(i);
        if (chosen_[i]) {
            box_.draw(0, canvas, r.x, r.y);
        }
        pictures_[i].draw(0, canvas, r.x + kPictureDx, r.y + kPictureDy);
    }
    foot_.draw(0, canvas, kLeft, bottom());
    const Rect all = allButton();
    all_.draw(all.contains(hoverX_, hoverY_) ? 1 : 0, canvas, all.x, all.y);
    const Rect close = closeButton();
    close_.draw(close.contains(hoverX_, hoverY_) ? 1 : 0, canvas, close.x, close.y);
    if (const auto colonize = colonizeButton()) {
        colonize_.draw(colonize->contains(hoverX_, hoverY_) ? 1 : 0, canvas, colonize->x, colonize->y);
    }

    TextStyle title;
    title.shades[1] = kTitleShade1;
    title.shades[2] = kTitleShade2;
    title.shades[3] = kTitleShade3;
    title.spacing = kTitleSpacing;
    title.shadowShades[1] = kOutline1;
    title.shadowShades[2] = kOutline2;
    title.shadowShades[3] = kOutline2;
    title.shadowAt = kOutlineOrder;
    const Font& big = fonts_.font(kTitleFont);
    big.draw(canvas, kTitleCentre - big.textWidth(title_, kTitleSpacing) / 2, kTitleY, title_, title);
    TextStyle status;
    status.shades[1] = kStatusShade1;
    status.shades[2] = kStatusShade2;
    status.spacing = kStatusSpacing;
    const Font& small = fonts_.font(kStatusFont);
    small.draw(canvas, kStatusCentre - small.textWidth(status_, kStatusSpacing) / 2, bottom() + kStatusDy, status_,
               status);
}

} // namespace hegemon
