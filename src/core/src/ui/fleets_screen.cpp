// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/fleets_screen.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/ship_design.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/build_list.hpp"
#include "hegemon/ui/fleet_window.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>

namespace hegemon {

namespace {

// Measured on the original; see docs/rules/fleets-screen.md.
constexpr const char* kArchive = "FLEET.LBX";
constexpr std::size_t kBackgroundEntry = 0, kScrollBarEntry = 1, kUpEntry = 2, kPreviousEntry = 3, kNextEntry = 4,
                      kDownEntry = 5, kAllEntry = 6, kRelocateEntry = 7, kScrapEntry = 8, kSupportEntry = 9,
                      kCombatEntry = 10, kReturnEntry = 12, kScrapOffEntry = 15, kLeadersOffEntry = 16,
                      kLitEntry = 17, kChosenEntry = 18, kFirstMarker = 19, kFirstStarDot = 34, kStarDots = 11;
constexpr std::size_t kUnownedDot = 8, kBlackHoleDot = 10; // counted from entry 34
constexpr int kScrollBarX = 605, kScrollBarY = 86;
constexpr int kThumbX = 607, kTrackTop = 88, kTrackBottom = 319;
constexpr std::uint8_t kTrack = 161;
constexpr std::size_t kPaletteEntry = 1; // of FONTS.LBX: the game's colours
// The grid: cells 62 apart from x 345 and 60 apart from y 55, the ship's
// picture 5 right and 4 below a cell's corner, the chosen one's frame 1
// left and 3 above it.
constexpr int kCellX = 345, kCellY = 55, kCellStepX = 62, kCellStepY = 60, kCellW = 59, kCellH = 57;
constexpr int kPictureDx = 5, kPictureDy = 4, kBoxDx = -1, kBoxDy = -3, kLitDx = 0, kLitDy = -1;
// The small map: a star's dot centred on (15 + x × 305 / width, 52 + y ×
// 182 / height); a fleet's mark from 1 right and 10 above its place.
constexpr int kMapX = 15, kMapY = 52, kMapWidth = 305, kMapHeight = 182;
constexpr int kMarkerDx = 1, kMarkerDy = -10;
// The bar under the map: font 1, centred on 166 at y 255, colours 73 and 80
// with a black outline.
constexpr int kTitleCentre = 166, kTitleY = 255;
constexpr std::uint8_t kTitleShade1 = 73, kTitleShade2 = 80;
// The panel, in colours 111 and 116: the name in font 3 at (18, 287), the
// crew, the shield and the beam values in font 2 at y 301, 315 and 329; the
// values end at 134 and 308 (assumed: right-aligned); the weapons and
// specials under their titles at y 357, in font 1 from y 370.
constexpr std::uint8_t kPanelShade1 = 111, kPanelShade2 = 116;
constexpr int kPanelX = 18, kRightX = 173, kNameY = 287, kCrewY = 301, kShieldY = 315, kBeamY = 329;
constexpr int kAttackEnd = 134, kDefenseEnd = 308, kListTitleY = 357, kListY = 370;
constexpr int kWeaponX = 23, kSpecialX = 188;
// Assumed until a ship with more is seen: 10 pixels a line.
constexpr int kListStep = 10;
// The help text of a colony ship: font 3, 15 pixels a line, centred on
// y 374, its lines at most 300 wide (between 281 and 302 measured).
constexpr int kHelpCentreY = 374, kHelpStep = 15, kHelpWrap = 300;
// HESTRNGS.LBX: the crews, the labels, the bar's words.
constexpr std::size_t kFirstCrew = 138, kAttackLabel = 153, kDefenseLabel = 154, kWeaponsLabel = 157,
                      kSpecialsLabel = 158, kFront = 149, kBack = 151, kOutpost = 258, kTransport = 260,
                      kColonyShip = 262, kFleetTitle = 305;
constexpr std::size_t kRaceNames = 645; // ESTRINGS.LBX
constexpr const char* kHelpArchive = "HELP.LBX";
constexpr std::size_t kHelpTextAt = 103;

TextStyle style(std::uint8_t shade1, std::uint8_t shade2, int spacing) {
    TextStyle s;
    s.shades[1] = shade1;
    s.shades[2] = s.shades[3] = shade2;
    s.spacing = spacing;
    return s;
}

std::string upper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

bool isSupport(ShipKind kind) {
    return kind == ShipKind::ColonyShip || kind == ShipKind::OutpostShip || kind == ShipKind::Transport;
}

const ShipDesign* designOf(const Game& game, const Ship& ship) {
    const auto& designs = game.players.at(static_cast<std::size_t>(ship.player)).designs;
    if (ship.kind == ShipKind::Scout) {
        return designs.empty() ? nullptr : &designs.front();
    }
    if (ship.kind == ShipKind::Combat && ship.design >= 0 && static_cast<std::size_t>(ship.design) < designs.size()) {
        return &designs[static_cast<std::size_t>(ship.design)];
    }
    return nullptr;
}

std::string help(const TextTable& table, const Parameters& p, std::string_view tech) {
    const auto id = p.integer("tech", {tech}, "id");
    return id > 0 && static_cast<std::size_t>(id) < table.size() ? table.at(static_cast<std::size_t>(id)) : std::string();
}

} // namespace

Rect FleetsScreen::cellRect(std::size_t cell) {
    const int c = static_cast<int>(cell) % kColumns, r = static_cast<int>(cell) / kColumns;
    return {kCellX + kCellStepX * c, kCellY + kCellStepY * r, kCellW, kCellH};
}

FleetsScreen::FleetsScreen(GameData& data, const Game& game, const TechTree& tree, int player)
    : data_(&data), game_(&game), player_(player),
      background_(Image::parse(data.archive(kArchive).entry(kBackgroundEntry))),
      scrollBar_(Image::parse(data.archive(kArchive).entry(kScrollBarEntry))),
      up_(Image::parse(data.archive(kArchive).entry(kUpEntry))),
      down_(Image::parse(data.archive(kArchive).entry(kDownEntry))),
      previous_(Image::parse(data.archive(kArchive).entry(kPreviousEntry))),
      next_(Image::parse(data.archive(kArchive).entry(kNextEntry))),
      all_(Image::parse(data.archive(kArchive).entry(kAllEntry))),
      relocate_(Image::parse(data.archive(kArchive).entry(kRelocateEntry))),
      scrap_(Image::parse(data.archive(kArchive).entry(kScrapEntry))),
      scrapOff_(Image::parse(data.archive(kArchive).entry(kScrapOffEntry))),
      leadersOff_(Image::parse(data.archive(kArchive).entry(kLeadersOffEntry))),
      supportButton_(Image::parse(data.archive(kArchive).entry(kSupportEntry))),
      combatButton_(Image::parse(data.archive(kArchive).entry(kCombatEntry))),
      return_(Image::parse(data.archive(kArchive).entry(kReturnEntry))),
      lit_(Image::parse(data.archive(kArchive).entry(kLitEntry))),
      box_(Image::parse(data.archive(kArchive).entry(kChosenEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto palette = data.archive("FONTS.LBX").entry(kPaletteEntry);
    if (palette.size() < 1024) {
        throw FormatError("FONTS.LBX: the game's colours are missing");
    }
    palette_.set(0, palette.subspan(0, 1024));
    const Player& owner = game.players.at(static_cast<std::size_t>(player));
    banner_ = owner.banner % 8;
    marker_ = Image::parse(data.archive(kArchive).entry(kFirstMarker + banner_));
    for (std::size_t i = 0; i < kStarDots; ++i) {
        starDots_.push_back(Image::parse(data.archive(kArchive).entry(kFirstStarDot + i)));
    }
    const auto& ships = data.archive("SHIPS.LBX");
    shipColours_ = Image::parse(ships.entry(banner_ * FleetWindow::kShipsPerBanner + FleetWindow::kShipColours));
    for (const Ship& s : game.ships) {
        const auto picture = static_cast<std::size_t>(
            std::clamp(shipPicture(game, s), 0, static_cast<int>(FleetWindow::kShipColours) - 1));
        pictures_.push_back(Image::parse(ships.entry(banner_ * FleetWindow::kShipsPerBanner + picture)));
    }
    strings_ = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    const auto race = kRaceNames + static_cast<std::size_t>(std::max(owner.race, 0));
    if (strings_.size() <= kFleetTitle || messages.size() <= race) {
        throw FormatError("too few messages in HESTRNGS.LBX or ESTRINGS.LBX");
    }
    race_ = messages[race];
    const Parameters& p = data.parameters();
    names_ = partNames(p, TechNames(data.archive("TECHNAME.LBX").entry(0), tree));
    const TextTable helpTexts = TextTable::parse(data.archive(kHelpArchive).entry(0), kHelpTextAt);
    colonyHelp_ = help(helpTexts, p, "colony_ship");
    outpostHelp_ = help(helpTexts, p, "outpost_ship");
    transportHelp_ = help(helpTexts, p, "transport");

    const Galaxy& galaxy = game.galaxy;
    const int width = std::max(1, galaxy.width), height = std::max(1, galaxy.height);
    for (std::size_t s = 0; s < galaxy.stars.size(); ++s) {
        const Star& star = galaxy.stars[s];
        std::size_t dot = kUnownedDot;
        if (star.colour == StarColour::BlackHole) {
            dot = kBlackHoleDot;
        } else {
            for (const Colony& c : game.colonies) {
                if (galaxy.planets.at(static_cast<std::size_t>(c.planet)).star == static_cast<int>(s) &&
                    (c.player == player || explored(owner, static_cast<int>(s)))) {
                    dot = game.players.at(static_cast<std::size_t>(c.player)).banner % 8;
                }
            }
        }
        stars_.push_back({kMapX + star.x * kMapWidth / width, kMapY + star.y * kMapHeight / height, dot});
    }
    for (const Fleet& f : hegemon::fleets(game)) {
        if (f.player == player) {
            fleets_.push_back(f);
        }
    }
    showFleet(0);
}

void FleetsScreen::showFleet(std::size_t fleet) {
    fleet_ = fleets_.empty() ? 0 : fleet % fleets_.size();
    top_ = 0;
    chosen_.reset();
    selected_.clear();
    title_.clear();
    if (!fleets_.empty()) {
        // "%s Fleet: " in capitals, then how many of each: the landing
        // ships, then the warships by hull size (assumed order, measured:
        // the colony ship before the frigates).
        std::string text = upper(formatMessage(strings_[kFleetTitle], {race_}));
        std::array<int, 3> support{};
        std::vector<int> hulls(names_.hulls.size());
        for (const std::size_t i : fleets_[fleet_].ships) {
            const Ship& s = game_->ships.at(i);
            if (s.kind == ShipKind::OutpostShip) {
                ++support[0];
            } else if (s.kind == ShipKind::Transport) {
                ++support[1];
            } else if (s.kind == ShipKind::ColonyShip) {
                ++support[2];
            } else if (const ShipDesign* d = designOf(*game_, s)) {
                const auto h = static_cast<std::size_t>(d->hull);
                if (h < hulls.size()) {
                    ++hulls[h];
                }
            }
        }
        const std::array<std::size_t, 3> words = {kOutpost, kTransport, kColonyShip};
        for (std::size_t k = 0; k < support.size(); ++k) {
            if (support[k] > 0) {
                text += formatMessage(strings_[words[k] + (support[k] == 1 ? 0 : 1)], {static_cast<long long>(support[k])});
            }
        }
        for (std::size_t h = 0; h < hulls.size(); ++h) {
            if (hulls[h] > 0) {
                text += std::to_string(hulls[h]) + " " + (hulls[h] == 1 ? names_.hulls[h] : names_.hullPlurals[h]) +
                        ", ";
            }
        }
        while (!text.empty() && (text.back() == ' ' || text.back() == ',')) {
            text.pop_back();
        }
        title_ = text;
    }
    refresh();
}

void FleetsScreen::refresh() {
    shown_.clear();
    if (!fleets_.empty()) {
        for (const std::size_t i : fleets_[fleet_].ships) {
            const bool support = isSupport(game_->ships.at(i).kind);
            if ((support && support_) || (!support && combat_)) {
                shown_.push_back(i);
            }
        }
    }
    if (chosen_ && std::find(shown_.begin(), shown_.end(), *chosen_) == shown_.end()) {
        chosen_.reset();
    }
    std::erase_if(selected_, [this](std::size_t i) { return std::find(shown_.begin(), shown_.end(), i) == shown_.end(); });
    scroll(0);
    describe();
}

void FleetsScreen::scroll(int rows) {
    const int total = (static_cast<int>(shown_.size()) + kColumns - 1) / kColumns;
    const int most = std::max(0, total - kRows);
    top_ = static_cast<std::size_t>(std::clamp(static_cast<int>(top_) + rows, 0, most));
}

void FleetsScreen::setSupport(bool on) {
    support_ = on;
    // Measured: one of the two stays on.
    combat_ = combat_ || !on;
    refresh();
}

void FleetsScreen::setCombat(bool on) {
    combat_ = on;
    support_ = support_ || !on;
    refresh();
}

void FleetsScreen::describe() {
    panel_.clear();
    if (!chosen_) {
        return;
    }
    const Ship& ship = game_->ships.at(*chosen_);
    const ShipDesign* design = designOf(*game_, ship);
    if (design == nullptr) {
        const std::string& text = ship.kind == ShipKind::ColonyShip    ? colonyHelp_
                                  : ship.kind == ShipKind::OutpostShip ? outpostHelp_
                                                                       : transportHelp_;
        const Font& font = fonts_.font(3);
        std::vector<std::string> lines;
        std::size_t at = 0;
        while (at < text.size()) {
            const std::size_t end = std::min(text.find(' ', at), text.size());
            const std::string word = text.substr(at, end - at);
            at = end + 1;
            if (word.empty()) {
                continue;
            }
            if (!lines.empty() && font.textWidth(lines.back() + " " + word, 2) <= kHelpWrap) {
                lines.back() += " " + word;
            } else {
                lines.push_back(word);
            }
        }
        const int y = kHelpCentreY - kHelpStep * static_cast<int>(lines.size()) / 2;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            panel_.push_back({kPanelX, y + kHelpStep * static_cast<int>(i), 3, 2, lines[i]});
        }
        return;
    }
    const Parameters& p = data_->parameters();
    const Player& owner = game_->players.at(static_cast<std::size_t>(ship.player));
    const int crew = crewLevel(p, ship.experience);
    const auto signedText = [](int v) { return (v < 0 ? "-" : "+") + std::to_string(std::abs(v)); };
    panel_.push_back({kPanelX, kNameY, 3, 2, designName(*data_, owner.race, *design)});
    panel_.push_back({kPanelX, kCrewY, 2, 1,
                      strings_[kFirstCrew + static_cast<std::size_t>(crew)] + " (" + std::to_string(ship.experience) +
                          " EP)"});
    panel_.push_back({kPanelX, kShieldY, 2, 1, nameIn(names_.shields, design->shield)});
    panel_.push_back({kPanelX, kBeamY, 2, 1, strings_[kAttackLabel]});
    panel_.push_back({kAttackEnd, kBeamY, 2, 1, signedText(beamAttack(p, owner.picks, *design, crew)), true});
    panel_.push_back({kRightX, kBeamY, 2, 1, strings_[kDefenseLabel]});
    panel_.push_back({kDefenseEnd, kBeamY, 2, 1, signedText(beamDefense(p, owner.picks, *design, crew)), true});
    panel_.push_back({kPanelX, kListTitleY, 2, 1, strings_[kWeaponsLabel]});
    panel_.push_back({kRightX, kListTitleY, 2, 1, strings_[kSpecialsLabel]});
    int y = kListY;
    for (const WeaponMount& m : design->weapons) {
        // Measured "(F)" for a forward arc; the others assumed.
        std::string arc;
        if (m.arc == kArcForward) {
            arc = strings_[kFront].substr(0, 1);
        } else if (m.arc == kArcForwardExtended) {
            arc = strings_[kFront].substr(0, 1) + "X";
        } else if (m.arc == kArcBack) {
            arc = strings_[kBack].substr(0, 1);
        } else if (m.arc == kArcBackExtended) {
            arc = strings_[kBack].substr(0, 1) + "X";
        } else {
            arc = "360";
        }
        panel_.push_back({kWeaponX, y, 1, 1, mountText(m, names_) + " (" + arc + ")"});
        y += kListStep;
    }
    y = kListY;
    for (int id = 1; id < static_cast<int>(names_.specials.size()); ++id) {
        if (design->hasSpecial(id)) {
            panel_.push_back({kSpecialX, y, 1, 1, names_.specials[static_cast<std::size_t>(id)]});
            y += kListStep;
        }
    }
}

std::vector<Rect> FleetsScreen::targets() const {
    std::vector<Rect> out = {kReturnButton,  kSupportButton,  kCombatButton, kAllButton,  kRelocateButton,
                             kScrapButton,   kPreviousButton, kNextButton,   kUpButton,   kDownButton};
    for (std::size_t c = 0; c < static_cast<std::size_t>(kColumns * kRows); ++c) {
        if (top_ * kColumns + c < shown_.size()) {
            out.push_back(cellRect(c));
        }
    }
    return out;
}

void FleetsScreen::press(int x, int y) {
    held_ = Rect{};
    for (const Rect& r : targets()) {
        if (r.contains(x, y)) {
            held_ = r;
        }
    }
}

FleetsScreen::Action FleetsScreen::release(int x, int y) {
    const Rect held = held_;
    held_ = Rect{};
    if (held.w == 0 || !held.contains(x, y)) {
        return Action::None;
    }
    const auto same = [&held](const Rect& r) { return held.x == r.x && held.y == r.y; };
    if (same(kReturnButton)) {
        return Action::Return;
    }
    if (same(kSupportButton)) {
        setSupport(!support_);
    } else if (same(kCombatButton)) {
        setCombat(!combat_);
    } else if (same(kPreviousButton) || same(kNextButton)) {
        if (!fleets_.empty()) {
            showFleet((fleet_ + (same(kNextButton) ? 1 : fleets_.size() - 1)) % fleets_.size());
        }
    } else if (same(kUpButton) || same(kDownButton)) {
        scroll(same(kUpButton) ? -1 : 1);
    } else if (same(kAllButton)) {
        // Measured: All lights every ship shown.
        selected_ = shown_;
    } else {
        // Measured: a click lights a ship or puts it out, and frames it.
        for (std::size_t c = 0; c < static_cast<std::size_t>(kColumns * kRows); ++c) {
            const std::size_t at = top_ * kColumns + c;
            if (at < shown_.size() && same(cellRect(c))) {
                const std::size_t ship = shown_[at];
                const auto it = std::find(selected_.begin(), selected_.end(), ship);
                if (it != selected_.end()) {
                    selected_.erase(it);
                } else {
                    selected_.push_back(ship);
                }
                chosen_ = ship;
                describe();
            }
        }
    }
    return Action::None;
}

std::pair<int, int> FleetsScreen::step(int x, int y, int dx, int dy) const {
    // The nearest target in the direction, the way along it counting less
    // than the way across.
    const std::vector<Rect> all = targets();
    const Rect* from = nullptr;
    for (const Rect& r : all) {
        if (r.contains(x, y)) {
            from = &r;
        }
    }
    const int fx = from != nullptr ? from->centerX() : x, fy = from != nullptr ? from->centerY() : y;
    const Rect* best = nullptr;
    int bestCost = 0;
    for (const Rect& r : all) {
        const int ax = r.centerX() - fx, ay = r.centerY() - fy;
        const int along = ax * dx + ay * dy;
        if (along <= 0) {
            continue;
        }
        const int across = std::abs(dx != 0 ? ay : ax);
        const int cost = along + 3 * across;
        if (best == nullptr || cost < bestCost) {
            best = &r;
            bestCost = cost;
        }
    }
    if (best == nullptr) {
        return {fx, fy};
    }
    return {best->centerX(), best->centerY()};
}

void FleetsScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = palette_;
    shipColours_.applyPalette(palette);
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            canvas.set(x, y, 0);
        }
    }
    background_.draw(0, canvas);
    const auto pressed = [this](const Rect& r) { return held_.w > 0 && held_.x == r.x && held_.y == r.y; };
    scrollBar_.draw(0, canvas, kScrollBarX, kScrollBarY);
    {
        // Measured with every ship shown: the bar's track (colour 161)
        // lit in columns of 165, 165, 164, 164, 163, 163, 162, 162, 161,
        // 161. Its part for a longer fleet is assumed.
        constexpr std::array<std::uint8_t, 10> kThumb = {165, 165, 164, 164, 163, 163, 162, 162, 161, 161};
        const int rows = (static_cast<int>(shown_.size()) + kColumns - 1) / kColumns;
        int from = kTrackTop, to = kTrackBottom;
        if (rows > kRows) {
            const int track = kTrackBottom - kTrackTop;
            from = kTrackTop + track * static_cast<int>(top_) / rows;
            to = from + track * kRows / rows;
        }
        for (int y = from; y < to; ++y) {
            for (std::size_t i = 0; i < kThumb.size(); ++i) {
                const int x = kThumbX + static_cast<int>(i);
                if (canvas.at(x, y) == kTrack) {
                    canvas.set(x, y, kThumb[i]);
                }
            }
        }
    }
    up_.draw(pressed(kUpButton) ? 1 : 0, canvas, kUpButton.x, kUpButton.y);
    down_.draw(pressed(kDownButton) ? 1 : 0, canvas, kDownButton.x, kDownButton.y);
    previous_.draw(pressed(kPreviousButton) ? 1 : 0, canvas, kPreviousButton.x, kPreviousButton.y);
    next_.draw(pressed(kNextButton) ? 1 : 0, canvas, kNextButton.x, kNextButton.y);
    all_.draw(pressed(kAllButton) ? 1 : 0, canvas, kAllButton.x, kAllButton.y);
    relocate_.draw(pressed(kRelocateButton) ? 1 : 0, canvas, kRelocateButton.x, kRelocateButton.y);
    // Measured: Scrap is grey while no ship is lit; Leaders while there
    // are none.
    if (!selected_.empty()) {
        scrap_.draw(pressed(kScrapButton) ? 1 : 0, canvas, kScrapButton.x, kScrapButton.y);
    } else {
        scrapOff_.draw(0, canvas, kScrapButton.x, kScrapButton.y);
    }
    leadersOff_.draw(0, canvas, kLeadersButton.x, kLeadersButton.y);
    supportButton_.draw(support_ ? 1 : 0, canvas, kSupportButton.x, kSupportButton.y);
    combatButton_.draw(combat_ ? 1 : 0, canvas, kCombatButton.x, kCombatButton.y);
    return_.draw(pressed(kReturnButton) ? 1 : 0, canvas, kReturnButton.x, kReturnButton.y);

    for (std::size_t c = 0; c < static_cast<std::size_t>(kColumns * kRows); ++c) {
        const std::size_t at = top_ * kColumns + c;
        if (at >= shown_.size()) {
            break;
        }
        const Rect r = cellRect(c);
        if (std::find(selected_.begin(), selected_.end(), shown_[at]) != selected_.end()) {
            lit_.draw(0, canvas, r.x + kLitDx, r.y + kLitDy);
        }
        if (chosen_ == shown_[at]) {
            box_.draw(0, canvas, r.x + kBoxDx, r.y + kBoxDy);
        }
        if (shown_[at] < pictures_.size()) {
            pictures_[shown_[at]].draw(0, canvas, r.x + kPictureDx, r.y + kPictureDy);
        }
    }

    for (const Dot& d : stars_) {
        const Image& dot = starDots_[d.entry];
        dot.draw(0, canvas, d.x - dot.width() / 2, d.y - dot.height() / 2);
    }
    const int width = std::max(1, game_->galaxy.width), height = std::max(1, game_->galaxy.height);
    for (const Fleet& f : fleets_) {
        const int x = kMapX + f.place.x * kMapWidth / width, y = kMapY + f.place.y * kMapHeight / height;
        marker_.draw(0, canvas, x + kMarkerDx, y + kMarkerDy);
    }

    const Font& small = fonts_.font(1);
    TextStyle title = style(kTitleShade1, kTitleShade2, 1);
    // Measured: a black outline all round.
    title.shadow = std::uint8_t{0};
    title.shadowAt = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    small.draw(canvas, kTitleCentre - small.textWidth(title_, 1) / 2, kTitleY, title_, title);
    for (const Line& l : panel_) {
        const Font& font = fonts_.font(l.font);
        const int x = l.right ? l.x - font.textWidth(l.text, l.spacing) : l.x;
        font.draw(canvas, x, l.y, l.text, style(kPanelShade1, kPanelShade2, l.spacing));
    }
}

} // namespace hegemon
