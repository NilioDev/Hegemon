// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/system_window.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/fleet.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/star-system.md), all BUFFER0.LBX.
constexpr const char* kArchive = "BUFFER0.LBX";
// The window: its frame (with the stars behind the system and the bar for
// the fleets), at the map's top left corner or in the middle of the map.
constexpr std::size_t kFrameEntry = 73;
constexpr int kCornerX = 22, kCornerY = 22, kMiddleX = 147, kMiddleY = 104;
// Close, from the frame's corner; frame 1 while held.
constexpr std::size_t kCloseEntry = 82;
constexpr int kCloseDx = 263, kCloseDy = 236;
// The ellipses of the five orbits and the asteroid belts (a frame for each
// orbit), 288 × 154 with the star in the middle, from the frame's corner.
constexpr std::size_t kOrbitEntry = 77, kBeltEntry = 91;
constexpr int kOrbitDx = 29, kOrbitDy = 59;
// The middle of the orbits, from the frame's corner, and the half widths
// and heights of the ellipses: 24 and 13 pixels more for each orbit out.
constexpr double kCentreDx = kOrbitDx + 143.5, kCentreDy = kOrbitDy + 76.5;
constexpr double kRadiusStepX = 24, kRadiusStepY = 13, kLine = 1.5;
// The star: one picture a colour, from the frame's corner.
constexpr std::size_t kStarEntry = 83;
constexpr int kStarDx = 156, kStarDy = 120;
// Planets: five sizes for each climate, from tiny; then the gas giant.
// Each has its turning frames and the colours 0 to 21.
constexpr std::size_t kPlanetEntry = 92, kGasGiantEntry = 142;
constexpr std::size_t kSizes = 5;
// Measured: a planet turns a frame every 7/60 second.
constexpr std::uint64_t kTurnFrameMs = 117;
// Their shading, one entry a size (the gas giant's assumed the huge one's),
// 31 frames round the star; each pixel says how dark the planet is there,
// 1 to 9. A dark pixel is the colour nearest to the planet's own times
// the level's factor, in thousandths (fitted to the original's pixels).
constexpr std::size_t kShadowEntry = 143, kShadowFrames = 31;
constexpr std::array<int, 10> kShadeFactor = {1000, 1000, 995, 920, 850, 780, 700, 610, 520, 450};
// The human's fleets there: an icon a banner, from the frame's corner.
constexpr std::size_t kFleetEntry = 273;
constexpr int kFleetDx = 18, kFleetDy = 241;
// Texts: the title centred in the bar on top, in font 4 or, when that is
// too wide, font 3; the wormhole in font 1 at the bottom right.
constexpr std::size_t kTitleFont = 4, kSmallTitleFont = 3, kWormholeFont = 1;
constexpr int kTitleCentre = 172, kTitleDy = 19, kSmallTitleDy = 20, kTitleSpacing = 2, kSmallTitleSpacing = 1;
constexpr int kTitleWidth = 300; // assumed: the bar is 315 wide
constexpr std::uint8_t kTitleShade1 = 60, kTitleShade2 = 65, kTitleShade3 = 64;
constexpr int kWormholeEnd = 330, kWormholeDy = 219, kWormholeSpacing = 1;
constexpr std::uint8_t kWormholeShade1 = 110, kWormholeShade2 = 115;
// HESTRNGS.LBX.
constexpr std::size_t kStarSystem = 362, kUnexplored = 363, kWormholeLinks = 364, kStableWormhole = 365;

std::string fill(const std::string& message, const std::string& text) {
    std::string s = message;
    const auto at = s.find("%s");
    if (at != std::string::npos) {
        s.replace(at, 2, text);
    }
    return s;
}

// The width of the drawn disc: the picture's columns with pixels.
int discWidth(const Image& picture) {
    Canvas c(picture.width(), picture.height(), 0);
    picture.draw(0, c);
    int right = 0;
    for (int y = 0; y < c.height(); ++y) {
        for (int x = 0; x < c.width(); ++x) {
            if (c.at(x, y) != 0) {
                right = std::max(right, x + 1);
            }
        }
    }
    return right;
}

// For each shading level, the screen colour each of a planet's colours is
// drawn in: its own colour (the first ones the picture carries, the rest
// the screen's) darkened, then the nearest of the screen's.
std::array<ColourMap, 10> shadeMaps(const Image& picture, const Palette& screen) {
    Palette own = screen;
    picture.applyPalette(own);
    std::array<ColourMap, 10> maps{};
    for (std::size_t level = 0; level < kShadeFactor.size(); ++level) {
        Palette darker = own;
        for (Color& c : darker.colors) {
            const auto scale = [&](std::uint8_t v) {
                const int six = (v >> 2) * kShadeFactor[level] / 1000;
                return static_cast<std::uint8_t>((six << 2) | (six >> 4));
            };
            c = {scale(c.r), scale(c.g), scale(c.b)};
        }
        maps[level] = nearestColours(darker, screen);
    }
    return maps;
}

} // namespace

int SystemWindow::planetAngle(int star, int orbit) {
    // A fixed scramble of the star and the orbit.
    unsigned v = static_cast<unsigned>(star) * 2654435761U + static_cast<unsigned>(orbit) * 40503U + 12345U;
    v ^= v >> 15;
    v *= 2246822519U;
    v ^= v >> 13;
    return static_cast<int>(v % 360U);
}

SystemWindow::SystemWindow(GameData& data, const Game& game, int star, Place place, std::size_t banner,
                           const std::string& title)
    : star_(star), place_(place), banner_(banner), frame_(Image::parse(data.archive(kArchive).entry(kFrameEntry))),
      close_(Image::parse(data.archive(kArchive).entry(kCloseEntry))),
      fleetIcon_(Image::parse(data.archive(kArchive).entry(kFleetEntry))),
      belt_(Image::parse(data.archive(kArchive).entry(kBeltEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings.size() <= kStableWormhole) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    const Galaxy& galaxy = game.galaxy;
    const Star& s = galaxy.stars.at(static_cast<std::size_t>(star));
    const Player& human = game.players.at(0);
    const bool known = explored(human, star);
    title_ = !title.empty() ? title : known ? fill(strings[kStarSystem], s.name) : strings[kUnexplored];
    smallTitle_ = fonts_.font(kTitleFont).textWidth(title_, kTitleSpacing) > kTitleWidth;
    if (s.wormhole >= 0) {
        wormhole_ = explored(human, s.wormhole)
                        ? fill(strings[kWormholeLinks], galaxy.stars.at(static_cast<std::size_t>(s.wormhole)).name)
                        : strings[kStableWormhole];
    }
    const auto colour = std::min<std::size_t>(static_cast<std::size_t>(s.colour), kStarColours - 1);
    starPicture_ = Image::parse(data.archive(kArchive).entry(kStarEntry + colour));
    for (const Ship& ship : game.ships) {
        if (ship.player == 0 && ship.star == star) {
            fleetHere_ = true;
        }
    }
    if (!known) {
        return;
    }
    for (std::size_t size = 0; size < kSizes; ++size) {
        shadows_.push_back(Image::parse(data.archive(kArchive).entry(kShadowEntry + size)));
    }
    for (std::size_t orbit = 0; orbit < kOrbits; ++orbit) {
        const int id = s.orbits[orbit];
        if (id < 0 || static_cast<std::size_t>(id) >= galaxy.planets.size()) {
            continue;
        }
        const Planet& p = galaxy.planets[static_cast<std::size_t>(id)];
        if (p.type == SatelliteType::AsteroidBelt) {
            beltOrbits_.push_back(static_cast<int>(orbit));
            continue;
        }
        orbits_.push_back(Image::parse(data.archive(kArchive).entry(kOrbitEntry + orbit)));
        Body b;
        b.planet = id;
        const auto size = std::min<std::size_t>(static_cast<std::size_t>(p.size), kSizes - 1);
        const std::size_t entry =
            p.type == SatelliteType::GasGiant
                ? kGasGiantEntry
                : kPlanetEntry + std::min<std::size_t>(static_cast<std::size_t>(p.climate), kClimates - 1) * kSizes + size;
        pictures_.push_back(Image::parse(data.archive(kArchive).entry(entry)));
        b.picture = pictures_.size() - 1;
        b.shadow = p.type == SatelliteType::GasGiant ? kSizes - 1 : size;
        b.disc = discWidth(pictures_.back());
        const int angle = planetAngle(star, static_cast<int>(orbit));
        const double a = angle * std::numbers::pi / 180;
        const double rx = kRadiusStepX * (static_cast<double>(orbit) + 2) - kLine;
        const double ry = kRadiusStepY * (static_cast<double>(orbit) + 2) - kLine;
        const int cx = static_cast<int>(std::lround(kCentreDx + rx * std::cos(a)));
        const int cy = static_cast<int>(std::lround(kCentreDy - ry * std::sin(a)));
        b.x = cx - (b.disc - 1) / 2;
        b.y = cy - (b.disc - 1) / 2;
        b.shadowFrame = static_cast<std::size_t>((angle * static_cast<int>(kShadowFrames) + 180) / 360) % kShadowFrames;
        bodies_.push_back(b);
    }
}

int SystemWindow::left() const { return place_ == Place::MapCorner ? kCornerX : kMiddleX; }

int SystemWindow::top() const { return place_ == Place::MapCorner ? kCornerY : kMiddleY; }

Rect SystemWindow::area() const { return {left(), top(), frame_.width(), frame_.height()}; }

Rect SystemWindow::closeButton() const {
    return {left() + kCloseDx, top() + kCloseDy, close_.width(), close_.height()};
}

std::optional<Rect> SystemWindow::planetRect(int planet) const {
    for (const Body& b : bodies_) {
        if (b.planet == planet) {
            return Rect{left() + b.x, top() + b.y, b.disc, b.disc};
        }
    }
    return std::nullopt;
}

std::optional<int> SystemWindow::planetAt(int x, int y) const {
    std::optional<int> found;
    for (const Body& b : bodies_) {
        if (planetRect(b.planet)->contains(x, y)) {
            found = b.planet; // the last drawn is on top
        }
    }
    return found;
}

void SystemWindow::press(int x, int y) { held_ = closeButton().contains(x, y) ? closeButton() : Rect{}; }

SystemWindow::Action SystemWindow::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    return held.w > 0 && held.contains(x, y) ? Action::Close : Action::None;
}

void SystemWindow::draw(Canvas& canvas, Palette& palette) const {
    const int x0 = left(), y0 = top();
    frame_.draw(0, canvas, x0, y0);
    for (const int orbit : beltOrbits_) {
        belt_.draw(std::min(static_cast<std::size_t>(orbit), belt_.frameCount() - 1), canvas, x0 + kOrbitDx, y0 + kOrbitDy);
    }
    for (const Image& ellipse : orbits_) {
        ellipse.draw(0, canvas, x0 + kOrbitDx, y0 + kOrbitDy);
    }
    if (starPicture_) {
        starPicture_->draw(0, canvas, x0 + kStarDx, y0 + kStarDy);
    }
    if (!shadedFor_ || shadedFor_->colors != palette.colors) {
        shades_.clear();
        for (const Image& picture : pictures_) {
            shades_.push_back(shadeMaps(picture, palette));
        }
        shadedFor_ = palette;
    }
    for (const Body& b : bodies_) {
        const Image& picture = pictures_[b.picture];
        const std::size_t frame =
            (static_cast<std::size_t>(b.planet) + static_cast<std::size_t>(time_ / kTurnFrameMs)) % picture.frameCount();
        Canvas disc(picture.width(), picture.height(), 0);
        picture.draw(frame, disc);
        const Image& shadow = shadows_[b.shadow];
        Canvas dark(shadow.width(), shadow.height(), 0);
        shadow.draw(b.shadowFrame % shadow.frameCount(), dark);
        for (int y = 0; y < disc.height(); ++y) {
            for (int x = 0; x < disc.width(); ++x) {
                const std::uint8_t v = disc.at(x, y);
                const int sx = x0 + b.x + x, sy = y0 + b.y + y;
                if (v == 0 || sx < 0 || sy < 0 || sx >= canvas.width() || sy >= canvas.height()) {
                    continue;
                }
                const std::uint8_t level =
                    x < dark.width() && y < dark.height() ? std::min<std::uint8_t>(dark.at(x, y), 9) : 0;
                canvas.set(sx, sy, shades_[b.picture][level][v]);
            }
        }
    }
    if (fleetHere_) {
        fleetIcon_.draw(std::min(banner_, fleetIcon_.frameCount() - 1), canvas, x0 + kFleetDx, y0 + kFleetDy);
    }
    const Rect close = closeButton();
    close_.draw(held_.w > 0 && close.contains(hoverX_, hoverY_) ? 1 : 0, canvas, close.x, close.y);

    TextStyle title;
    title.shades[1] = kTitleShade1;
    title.shades[2] = kTitleShade2;
    title.shades[3] = kTitleShade3;
    const std::size_t font = smallTitle_ ? kSmallTitleFont : kTitleFont;
    title.spacing = smallTitle_ ? kSmallTitleSpacing : kTitleSpacing;
    const Font& f = fonts_.font(font);
    f.draw(canvas, x0 + kTitleCentre - (f.textWidth(title_, title.spacing) - title.spacing) / 2,
           y0 + (smallTitle_ ? kSmallTitleDy : kTitleDy), title_, title);
    if (!wormhole_.empty()) {
        TextStyle w;
        w.shades[1] = kWormholeShade1;
        w.shades[2] = kWormholeShade2;
        w.spacing = kWormholeSpacing;
        const Font& small = fonts_.font(kWormholeFont);
        small.draw(canvas, x0 + kWormholeEnd - small.textWidth(wormhole_, kWormholeSpacing), y0 + kWormholeDy,
                   wormhole_, w);
    }
}

} // namespace hegemon
