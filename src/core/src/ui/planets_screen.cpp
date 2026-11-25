// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/planets_screen.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/fleet.hpp"

#include <algorithm>
#include <cstdlib>

namespace hegemon {

namespace {

// Measured on the original; see docs/rules/planets-screen.md.
constexpr const char* kArchive = "PLNTSUM.LBX";
constexpr std::size_t kBackgroundEntry = 0, kFirstSortEntry = 1, kFirstFilterEntry = 4, kColonyEntry = 9,
                      kOutpostEntry = 10, kUpEntry = 11, kDownEntry = 12, kReturnEntry = 14, kFirstStarDot = 15,
                      kStarDots = 11, kFirstIcon = 26, kFirstMarker = 76;
constexpr std::size_t kUnownedDot = 8, kBlackHoleDot = 10; // counted from entry 15
constexpr std::array<int, PlanetsScreen::kSorts> kSortX = {441, 501, 567};
constexpr std::array<int, PlanetsScreen::kSorts> kSortWidth = {60, 66, 57};
constexpr int kSortY = 200, kSortHeight = 24;
constexpr std::array<int, PlanetsScreen::kFilters> kFilterY = {266, 289, 312, 335, 358};
constexpr std::array<int, PlanetsScreen::kFilters> kFilterHeight = {21, 22, 22, 22, 20};
constexpr int kFilterX = 441, kFilterWidth = 183;
// The rows: their tops, from x 18 to 416.
constexpr std::array<int, PlanetsScreen::kRowsShown> kRowTop = {36, 91, 145, 200, 255, 310, 365, 420};
constexpr int kThumbX = 423, kTrackTop = 38, kTrackBottom = 445;
constexpr int kRowX = 18, kRowWidth = 398, kRowHeight = 50;
// In a row: the planet centred on (61, +21) with its name in font 2
// centred on 61 at +29; the climate, minerals and size in font 2 centred on
// 140, 311 and 386 at +12, with a line in font 1 at +28 under each (the
// food centred on 141); the gravity in font 2 centred on 217 at +18.
constexpr int kIconCentreX = 61, kIconCentreY = 21, kNameY = 29, kTopLine = 12, kSecondLine = 28, kGravityY = 18;
constexpr int kClimateCentre = 140, kFoodCentre = 141, kGravityCentre = 217, kMineralsCentre = 311, kSizeCentre = 386;
// The mark of a ship sent there (one for each banner colour), at (90, +38).
constexpr int kMarkerX = 90, kMarkerY = 38;
constexpr std::uint8_t kShade1 = 64, kShade2 = 65;
// The small map: a star's dot centred on (443 + x × 180 / width,
// 17 + y × 116 / height); the planet chosen has a square of colour 212
// around its star, 9 pixels wide. Under it, how far the ship is, in font 2
// centred on 533 at y 150, colours 219 and 222.
constexpr int kMapX = 443, kMapY = 17, kMapWidth = 180, kMapHeight = 116;
constexpr std::uint8_t kChosenColour = 212, kTurnsShade1 = 219, kTurnsShade2 = 222;
constexpr int kTurnsCentre = 533, kTurnsY = 150;
// Assumed until measured: ships reach planets up to 4 parsecs from the
// player's colonies.
constexpr int kRange = 4;
// The messages: ESTRINGS.LBX for the words, HESTRNGS.LBX for the rest.
constexpr std::array<std::size_t, 5> kSizeMessages = {683, 480, 371, 360, 323};
constexpr std::array<std::size_t, 10> kClimateMessages = {539, 719, 720, 721, 722, 399, 501, 184, 723, 303};
constexpr std::array<std::size_t, 5> kMineralMessages = {684, 423, 685, 450, 686};
constexpr std::array<std::size_t, 3> kGravityMessages = {687, 688, 689};
constexpr std::size_t kLastMessage = 723;
constexpr std::size_t kTurnText = 105, kTurnsText = 106, kFoodText = 321, kMaxPopText = 323, kProductionText = 338,
                      kPrimeText = 160;

TextStyle style(std::uint8_t shade1, std::uint8_t shade2, int spacing) {
    TextStyle s;
    s.shades[1] = shade1;
    s.shades[2] = s.shades[3] = shade2;
    s.spacing = spacing;
    return s;
}

void centred(const Font& font, Canvas& canvas, int centre, int y, const std::string& text, const TextStyle& s) {
    font.draw(canvas, centre - font.textWidth(text, s.spacing) / 2, y, text, s);
}

} // namespace

Rect PlanetsScreen::sortRect(Sort sort) {
    const auto i = static_cast<std::size_t>(sort);
    return {kSortX.at(i), kSortY, kSortWidth.at(i), kSortHeight};
}

Rect PlanetsScreen::filterRect(Filter filter) {
    const auto i = static_cast<std::size_t>(filter);
    return {kFilterX, kFilterY.at(i), kFilterWidth, kFilterHeight.at(i)};
}

Rect PlanetsScreen::rowRect(std::size_t row) {
    return {kRowX, kRowTop.at(row), kRowWidth, kRowHeight};
}

PlanetsScreen::PlanetsScreen(GameData& data, const Game& game, const Economy& economy, int player)
    : game_(&game), parameters_(&data.parameters()), player_(player),
      background_(Image::parse(data.archive(kArchive).entry(kBackgroundEntry))),
      return_(Image::parse(data.archive(kArchive).entry(kReturnEntry))),
      colony_(Image::parse(data.archive(kArchive).entry(kColonyEntry))),
      outpost_(Image::parse(data.archive(kArchive).entry(kOutpostEntry))),
      up_(Image::parse(data.archive(kArchive).entry(kUpEntry))),
      down_(Image::parse(data.archive(kArchive).entry(kDownEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    for (std::size_t i = 0; i < kSorts; ++i) {
        sorts_[i] = Image::parse(data.archive(kArchive).entry(kFirstSortEntry + i));
    }
    for (std::size_t i = 0; i < kFilters; ++i) {
        filterButtons_[i] = Image::parse(data.archive(kArchive).entry(kFirstFilterEntry + i));
    }
    for (std::size_t i = 0; i < kStarDots; ++i) {
        starDots_.push_back(Image::parse(data.archive(kArchive).entry(kFirstStarDot + i)));
    }
    const Player& owner = game.players.at(static_cast<std::size_t>(player));
    marker_ = Image::parse(data.archive(kArchive).entry(kFirstMarker + owner.banner % 8));
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    if (strings.size() <= kProductionText || messages.size() <= kLastMessage) {
        throw FormatError("too few messages in HESTRNGS.LBX or ESTRINGS.LBX");
    }
    turnPattern_ = strings[kTurnText];
    turnsPattern_ = strings[kTurnsText];

    // The planets of the stars the player explored that nobody lives on.
    const Galaxy& galaxy = game.galaxy;
    for (std::size_t s = 0; s < galaxy.stars.size(); ++s) {
        const int star = static_cast<int>(s);
        if (!explored(owner, star)) {
            continue;
        }
        bool enemy = std::any_of(galaxy.monsters.begin(), galaxy.monsters.end(),
                                 [&](const Monster& m) { return m.star == star; });
        int nearest = -1;
        for (const Colony& c : game.colonies) {
            const Planet& at = galaxy.planets.at(static_cast<std::size_t>(c.planet));
            if (at.star == star && c.player != player) {
                enemy = true;
            }
            if (c.player == player) {
                const Star& home = galaxy.stars.at(static_cast<std::size_t>(at.star));
                const int d = mapDistance({home.x, home.y}, {galaxy.stars[s].x, galaxy.stars[s].y});
                nearest = nearest < 0 ? d : std::min(nearest, d);
            }
        }
        for (const int planet : colonySites(game, star)) {
            const Planet& p = galaxy.planets[static_cast<std::size_t>(planet)];
            Entry e;
            e.row.planet = planet;
            e.row.star = star;
            e.row.name = planetName(galaxy, planet, strings[kPrimeText]);
            e.climate = static_cast<int>(p.climate);
            e.minerals = static_cast<int>(p.minerals);
            e.size = static_cast<int>(p.size);
            const auto climate = static_cast<std::size_t>(p.climate) % kClimateMessages.size();
            const auto size = static_cast<std::size_t>(p.size) % kSizeMessages.size();
            e.row.climate = messages[kClimateMessages[climate]];
            e.row.food = formatMessage(strings[kFoodText], {static_cast<long long>(economy.farmerFood2(owner, p) / 2)});
            e.row.gravity = messages[kGravityMessages[static_cast<std::size_t>(p.gravity) % kGravityMessages.size()]];
            e.row.minerals = messages[kMineralMessages[static_cast<std::size_t>(p.minerals) % kMineralMessages.size()]];
            e.row.production = formatMessage(strings[kProductionText],
                                             {static_cast<long long>(economy.workerProduction(owner, p))});
            e.row.size = messages[kSizeMessages[size]];
            e.row.population = formatMessage(strings[kMaxPopText],
                                             {static_cast<long long>(economy.maxPopulation(owner, p))});
            e.row.shipSent = std::any_of(game.ships.begin(), game.ships.end(), [&](const Ship& ship) {
                return ship.player == player && ship.landOn == planet;
            });
            e.gravityNormal = p.gravity == Gravity::Normal;
            // Assumed: toxic, radiated and barren planets are the hostile ones.
            e.hostile = p.climate == Climate::Toxic || p.climate == Climate::Radiated || p.climate == Climate::Barren;
            e.enemy = enemy;
            e.inRange = nearest >= 0 && nearest <= kRange * kParsec;
            e.order = entries_.size();
            const std::size_t icon = kFirstIcon + 5 * climate + size;
            e.icon = Image::parse(data.archive(kArchive).entry(icon));
            entries_.push_back(std::move(e));
        }
    }

    const int width = std::max(1, galaxy.width), height = std::max(1, galaxy.height);
    for (std::size_t s = 0; s < galaxy.stars.size(); ++s) {
        const Star& star = galaxy.stars[s];
        std::size_t dot = kUnownedDot;
        if (star.colour == StarColour::BlackHole) {
            dot = kBlackHoleDot;
        } else {
            // A star with a colony the player knows of in its owner's colour.
            for (const Colony& c : game.colonies) {
                if (galaxy.planets.at(static_cast<std::size_t>(c.planet)).star == static_cast<int>(s) &&
                    (c.player == player || explored(owner, static_cast<int>(s)))) {
                    dot = game.players.at(static_cast<std::size_t>(c.player)).banner % 8;
                }
            }
        }
        stars_.push_back({kMapX + star.x * kMapWidth / width, kMapY + star.y * kMapHeight / height, dot});
    }
    // Measured: with a colony ship, a click sends one; else an outpost ship.
    mode_ = hasShip(Mode::Colony) ? Mode::Colony : hasShip(Mode::Outpost) ? Mode::Outpost : Mode::None;
    refresh();
}

bool PlanetsScreen::hasShip(Mode mode) const {
    const ShipKind kind = mode == Mode::Colony ? ShipKind::ColonyShip : ShipKind::OutpostShip;
    return mode != Mode::None && std::any_of(game_->ships.begin(), game_->ships.end(), [&](const Ship& s) {
               return s.player == player_ && s.kind == kind;
           });
}

void PlanetsScreen::refresh() {
    order_.clear();
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const Entry& e = entries_[i];
        const bool hidden = (filter(Filter::NoEnemy) && e.enemy) ||
                            (filter(Filter::NormalGravity) && !e.gravityNormal) ||
                            (filter(Filter::NonHostile) && e.hostile) ||
                            (filter(Filter::Abundance) && e.minerals < static_cast<int>(Minerals::Abundant)) ||
                            (filter(Filter::InRange) && !e.inRange);
        if (!hidden) {
            order_.push_back(i);
        }
    }
    // Measured: the best first (radiated before toxic, large before
    // medium); equal ones in the galaxy's order (assumed).
    const auto key = [this](std::size_t i) {
        const Entry& e = entries_[i];
        return sort_ == Sort::Climate ? e.climate : sort_ == Sort::Minerals ? e.minerals : e.size;
    };
    std::stable_sort(order_.begin(), order_.end(), [&](std::size_t a, std::size_t b) { return key(a) > key(b); });
    shown_.clear();
    for (const std::size_t i : order_) {
        shown_.push_back(entries_[i].row);
    }
    scroll(0);
}

void PlanetsScreen::setSort(Sort sort) {
    sort_ = sort;
    refresh();
}

void PlanetsScreen::setFilter(Filter f, bool on) {
    filters_[static_cast<std::size_t>(f)] = on;
    refresh();
}

void PlanetsScreen::scroll(int rows) {
    const int most = std::max(0, static_cast<int>(order_.size()) - static_cast<int>(kRowsShown));
    top_ = static_cast<std::size_t>(std::clamp(static_cast<int>(top_) + rows, 0, most));
}

std::optional<std::pair<std::size_t, int>> PlanetsScreen::nearestShip(Mode mode, int star) const {
    const ShipKind kind = mode == Mode::Colony ? ShipKind::ColonyShip : ShipKind::OutpostShip;
    const Star& target = game_->galaxy.stars.at(static_cast<std::size_t>(star));
    std::optional<std::pair<std::size_t, int>> best;
    bool bestFree = false;
    for (std::size_t i = 0; i < game_->ships.size(); ++i) {
        const Ship& s = game_->ships[i];
        if (s.player != player_ || s.kind != kind || mode == Mode::None) {
            continue;
        }
        const bool free = s.landOn < 0;
        const int turns = travelTurns(shipPlace(*game_, s), {target.x, target.y}, shipSpeed(*parameters_, *game_, s));
        if (!best || (free && !bestFree) || (free == bestFree && turns < best->second)) {
            best = {i, turns};
            bestFree = free;
        }
    }
    return best;
}

void PlanetsScreen::press(int x, int y) {
    held_ = Rect{};
    std::vector<Rect> buttons = {kReturnButton, kColonyButton, kOutpostButton, kUpButton, kDownButton};
    for (std::size_t s = 0; s < kSorts; ++s) {
        buttons.push_back(sortRect(static_cast<Sort>(s)));
    }
    for (std::size_t f = 0; f < kFilters; ++f) {
        buttons.push_back(filterRect(static_cast<Filter>(f)));
    }
    for (std::size_t r = 0; r < kRowsShown && top_ + r < order_.size(); ++r) {
        buttons.push_back(rowRect(r));
    }
    for (const Rect& b : buttons) {
        if (b.contains(x, y)) {
            held_ = b;
        }
    }
}

PlanetsScreen::Action PlanetsScreen::release(int x, int y) {
    const Rect held = held_;
    held_ = Rect{};
    if (held.w == 0 || !held.contains(x, y)) {
        return {};
    }
    const auto same = [&held](const Rect& r) { return held.x == r.x && held.y == r.y; };
    if (same(kReturnButton)) {
        return {Action::Kind::Return};
    }
    if (same(kColonyButton) || same(kOutpostButton)) {
        // Measured: a click lights the button, and another puts it out.
        const Mode m = same(kColonyButton) ? Mode::Colony : Mode::Outpost;
        if (hasShip(m)) {
            mode_ = mode_ == m ? Mode::None : m;
        }
        return {};
    }
    if (same(kUpButton) || same(kDownButton)) {
        scroll(same(kUpButton) ? -1 : 1);
        return {};
    }
    for (std::size_t s = 0; s < kSorts; ++s) {
        if (same(sortRect(static_cast<Sort>(s)))) {
            setSort(static_cast<Sort>(s));
            return {};
        }
    }
    for (std::size_t f = 0; f < kFilters; ++f) {
        if (same(filterRect(static_cast<Filter>(f)))) {
            setFilter(static_cast<Filter>(f), !filters_[f]);
            return {};
        }
    }
    for (std::size_t r = 0; r < kRowsShown && top_ + r < order_.size(); ++r) {
        if (!same(rowRect(r))) {
            continue;
        }
        const Row& row = entries_[order_[top_ + r]].row;
        chosen_ = row.planet;
        turns_.clear();
        const auto ship = nearestShip(mode_, row.star);
        if (!ship) {
            return {};
        }
        const std::string& star = game_->galaxy.stars.at(static_cast<std::size_t>(row.star)).name;
        turns_ = formatMessage(ship->second == 1 ? turnPattern_ : turnsPattern_,
                               {static_cast<long long>(ship->second), star});
        // Measured: a click on a planet sends the ship at once.
        return {Action::Kind::Send, ship->first, row.planet, game_->ships[ship->first].landOn >= 0};
    }
    return {};
}

void PlanetsScreen::shipSent(int planet) {
    for (Entry& e : entries_) {
        e.row.shipSent = e.row.shipSent || e.row.planet == planet;
    }
    refresh();
}

std::pair<int, int> PlanetsScreen::step(int x, int y, int dx, int dy) const {
    std::vector<std::vector<Rect>> lines;
    for (std::size_t r = 0; r < kRowsShown && top_ + r < order_.size(); ++r) {
        lines.push_back({rowRect(r)});
    }
    std::vector<Rect> sorts;
    for (std::size_t s = 0; s < kSorts; ++s) {
        sorts.push_back(sortRect(static_cast<Sort>(s)));
    }
    lines.push_back(sorts);
    for (std::size_t f = 0; f < kFilters; ++f) {
        lines.push_back({filterRect(static_cast<Filter>(f))});
    }
    lines.push_back({kColonyButton});
    lines.push_back({kOutpostButton});
    lines.push_back({kReturnButton});
    std::size_t line = 0, at = 0;
    int best = -1;
    for (std::size_t l = 0; l < lines.size(); ++l) {
        for (std::size_t i = 0; i < lines[l].size(); ++i) {
            const int d = std::abs(lines[l][i].centerX() - x) + std::abs(lines[l][i].centerY() - y);
            if (best < 0 || d < best) {
                best = d;
                line = l;
                at = i;
            }
        }
    }
    // Left and right also go between the list and the buttons.
    const std::size_t rows = lines.size() - 3 - kFilters - 1;
    if (dx != 0 && lines[line].size() == 1) {
        if (dx > 0 && line < rows) {
            line = rows; // to the sort buttons
            at = 0;
        } else if (dx < 0 && line >= rows && rows > 0) {
            line = 0;
            at = 0;
        }
        dx = 0;
    }
    if (dy != 0) {
        line = static_cast<std::size_t>(std::clamp(static_cast<int>(line) + dy, 0, static_cast<int>(lines.size()) - 1));
        at = std::min(at, lines[line].size() - 1);
    }
    if (dx != 0) {
        at = static_cast<std::size_t>(std::clamp(static_cast<int>(at) + dx, 0, static_cast<int>(lines[line].size()) - 1));
    }
    return {lines[line][at].centerX(), lines[line][at].centerY()};
}

void PlanetsScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    // Parts of the picture are see-through: black under them.
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            canvas.set(x, y, 0);
        }
    }
    background_.draw(0, canvas);
    const auto pressed = [this](const Rect& r) { return held_.w > 0 && held_.x == r.x && held_.y == r.y; };
    const auto chosenSort = static_cast<std::size_t>(sort_);
    for (std::size_t s = 0; s < kSorts; ++s) {
        sorts_[s].draw(s == chosenSort ? 1 : 0, canvas, kSortX[s], kSortY);
    }
    for (std::size_t f = 0; f < kFilters; ++f) {
        filterButtons_[f].draw(filters_[f] ? 1 : 0, canvas, kFilterX, kFilterY[f]);
    }
    // Measured: the frames are no ship, a ship, and chosen.
    colony_.draw(!hasShip(Mode::Colony) ? 0 : mode_ == Mode::Colony ? 2 : 1, canvas, kColonyButton.x, kColonyButton.y);
    outpost_.draw(!hasShip(Mode::Outpost) ? 0 : mode_ == Mode::Outpost ? 2 : 1, canvas, kOutpostButton.x,
                  kOutpostButton.y);
    return_.draw(pressed(kReturnButton) ? 1 : 0, canvas, kReturnButton.x, kReturnButton.y);
    up_.draw(pressed(kUpButton) ? 1 : 0, canvas, kUpButton.x, kUpButton.y);
    down_.draw(pressed(kDownButton) ? 1 : 0, canvas, kDownButton.x, kDownButton.y);
    {
        // Measured with every row shown: a grey bar fills the track, its
        // columns in colours 150, 149, 149, 148, 148, 147, 147, 146, 146.
        // Its part for a longer list is assumed.
        constexpr std::array<std::uint8_t, 9> kThumb = {150, 149, 149, 148, 148, 147, 147, 146, 146};
        const int total = static_cast<int>(order_.size());
        int from = kTrackTop, to = kTrackBottom;
        if (total > static_cast<int>(kRowsShown)) {
            const int track = kTrackBottom - kTrackTop;
            from = kTrackTop + track * static_cast<int>(top_) / total;
            to = from + track * static_cast<int>(kRowsShown) / total;
        }
        for (int y = from; y < to; ++y) {
            for (std::size_t i = 0; i < kThumb.size(); ++i) {
                canvas.set(kThumbX + static_cast<int>(i), y, kThumb[i]);
            }
        }
    }

    const Font& big = fonts_.font(2);
    const Font& small = fonts_.font(1);
    const TextStyle bigStyle = style(kShade1, kShade2, 2);
    const TextStyle smallStyle = style(kShade1, kShade2, 1);
    for (std::size_t r = 0; r < kRowsShown && top_ + r < order_.size(); ++r) {
        const Entry& e = entries_[order_[top_ + r]];
        const int top = kRowTop[r];
        e.icon.draw(0, canvas, kIconCentreX - e.icon.width() / 2, top + kIconCentreY - e.icon.height() / 2);
        centred(big, canvas, kIconCentreX, top + kNameY, e.row.name, bigStyle);
        centred(big, canvas, kClimateCentre, top + kTopLine, e.row.climate, bigStyle);
        centred(small, canvas, kFoodCentre, top + kSecondLine, e.row.food, smallStyle);
        centred(big, canvas, kGravityCentre, top + kGravityY, e.row.gravity, bigStyle);
        centred(big, canvas, kMineralsCentre, top + kTopLine, e.row.minerals, bigStyle);
        centred(small, canvas, kMineralsCentre, top + kSecondLine, e.row.production, smallStyle);
        centred(big, canvas, kSizeCentre, top + kTopLine, e.row.size, bigStyle);
        centred(small, canvas, kSizeCentre, top + kSecondLine, e.row.population, smallStyle);
        if (e.row.shipSent) {
            marker_.draw(0, canvas, kMarkerX, top + kMarkerY);
        }
    }

    for (const Dot& d : stars_) {
        const Image& dot = starDots_[d.entry];
        dot.draw(0, canvas, d.x - dot.width() / 2, d.y - dot.height() / 2);
    }
    if (chosen_) {
        const int star = game_->galaxy.planets.at(static_cast<std::size_t>(*chosen_)).star;
        const Dot& d = stars_.at(static_cast<std::size_t>(star));
        for (int i = -4; i <= 4; ++i) {
            for (const int edge : {-4, 4}) {
                for (const auto& [px, py] : {std::pair{d.x + i, d.y + edge}, std::pair{d.x + edge, d.y + i}}) {
                    if (px >= 0 && py >= 0 && px < canvas.width() && py < canvas.height()) {
                        canvas.set(px, py, kChosenColour);
                    }
                }
            }
        }
        centred(big, canvas, kTurnsCentre, kTurnsY, turns_, style(kTurnsShade1, kTurnsShade2, 2));
    }
}

} // namespace hegemon
