// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/product_picture.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/ui/rect.hpp"

#include <algorithm>
#include <string>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colony-screen.md). The colony box
// shows each picture 334 pixels right of and 21 below where the build list
// does (a building 335 right and 13 below), cut off at its frame.
constexpr Rect kBuildListClip = {202, 14, 89, 93};
constexpr Rect kColonyClip = {522, 15, 114, 85};
constexpr int kColonyDx = 334, kColonyDy = 21, kBuildingColonyDx = 335, kBuildingColonyDy = 13;
// Ships: SHIPS.LBX at (218, 32).
constexpr int kShipX = 218, kShipY = 32;
// Ships are drawn for the colours of FONTS.LBX entry 1.
constexpr std::size_t kShipPalette = 1;
constexpr std::size_t kPlayerColours = 192;
constexpr std::array<std::uint8_t, 16> kShipGreys = {5, 9, 13, 17, 20, 24, 28, 32, 36, 40, 43, 47, 51, 55, 59, 63};
constexpr std::size_t kColonyShipPicture = 45, kOutpostShipPicture = 46, kTransportPicture = 47,
                      kFreightersPicture = 48;
// A spy: RACEICON.LBX entry 13 × race + 11 at (230, 42).
constexpr int kSpyX = 230, kSpyY = 42;
// Trade goods: COLONY2.LBX entry 7 at (235, 54); the colony box shows none.
constexpr int kTradeGoodsX = 235, kTradeGoodsY = 54;
constexpr std::size_t kTradeGoodsPicture = 7;
// A building, and housing: its landscape picture at place 30 (housing
// picture 13), what is not shadow centred on x 244 (left = 244 − width / 2)
// with its bottom row at 92.
constexpr int kBuildingPlace = 30, kHousingPicture = 13;
constexpr int kBuildingCentre = 244, kBuildingBottom = 92;
constexpr std::size_t kPlaces = 36, kPicturesPerFile = 10;
constexpr std::uint8_t kShadow = 240, kShadow2 = 241;

int six(const Color& c, int channel) {
    const int v = channel == 0 ? c.r : channel == 1 ? c.g : c.b;
    return (v * 63 + 127) / 255;
}

} // namespace

ProductPicture::ProductPicture(GameData& data, std::string_view product, int race) {
    const Parameters& p = data.parameters();
    std::optional<Image> image;
    std::size_t frame = 0;
    int x = 0, y = 0;
    bool colony = true, building = false;
    const auto ship = [&](std::size_t entry) {
        image = Image::parse(data.archive("SHIPS.LBX").entry(entry));
        const auto colours = data.archive("FONTS.LBX").entry(kShipPalette);
        if (colours.size() < 1024) {
            throw FormatError("FONTS.LBX: a palette is missing");
        }
        own_.emplace();
        own_->set(0, colours.subspan(0, 1024));
        // Measured: the box shows ships in greys, the player colours 192 to
        // 223 as two ramps of the 16 greys below, colour 1 the darkest and
        // 31 white.
        std::array<std::uint8_t, 4 * 32> greys{};
        for (std::size_t k = 0; k < 32; ++k) {
            greys[4 * k + 1] = greys[4 * k + 2] = greys[4 * k + 3] = kShipGreys[k % kShipGreys.size()];
        }
        own_->set(kPlayerColours, greys);
        own_->colors[1] = own_->colors[kPlayerColours];
        own_->colors[31] = own_->colors[kPlayerColours + kShipGreys.size() - 1];
        x = kShipX;
        y = kShipY;
    };
    int picture = -1;
    if (product == kColonyShip) {
        ship(kColonyShipPicture);
    } else if (product == kOutpostShip) {
        ship(kOutpostShipPicture);
    } else if (product == kTransportShip) {
        ship(kTransportPicture);
    } else if (product == kFreighters) {
        ship(kFreightersPicture);
    } else if (product == kSpy) {
        image = Image::parse(data.archive("RACEICON.LBX").entry(13 * static_cast<std::size_t>(std::max(race, 0)) + 11));
        x = kSpyX;
        y = kSpyY;
    } else if (product == kTradeGoods) {
        image = Image::parse(data.archive("COLONY2.LBX").entry(kTradeGoodsPicture));
        x = kTradeGoodsX;
        y = kTradeGoodsY;
        colony = false;
    } else if (product == kHousing) {
        picture = kHousingPicture;
    } else {
        const ParameterDefinition* d = p.definition("building");
        if (d != nullptr && !d->rowNames.empty()) {
            const auto& rows = d->rowNames[0];
            const auto it = std::find(rows.begin(), rows.end(), product);
            if (it != rows.end()) {
                picture = static_cast<int>(it - rows.begin());
            }
        }
    }
    if (picture >= 0) {
        const std::size_t at = static_cast<std::size_t>(picture) * kPlaces + kBuildingPlace;
        const std::string file = "BLDG" + std::to_string(at / (kPlaces * kPicturesPerFile)) + ".LBX";
        if (!data.hasFile(file) || at % (kPlaces * kPicturesPerFile) >= data.archive(file).size()) {
            return;
        }
        image = Image::parse(data.archive(file).entry(at % (kPlaces * kPicturesPerFile)));
        building = true;
    }
    if (!image || image->frameCount() == 0) {
        return;
    }
    Canvas a(image->width(), image->height(), 0), b(image->width(), image->height(), 1);
    image->draw(frame, a);
    image->draw(frame, b);
    int x0 = image->width(), x1 = -1, y0 = image->height(), y1 = -1; // all it draws
    int s0 = image->width(), s1 = -1, sBottom = -1;                  // what is not shadow
    for (int row = 0; row < image->height(); ++row) {
        for (int col = 0; col < image->width(); ++col) {
            if (a.at(col, row) != b.at(col, row)) {
                continue;
            }
            x0 = std::min(x0, col);
            x1 = std::max(x1, col);
            y0 = std::min(y0, row);
            y1 = std::max(y1, row);
            if (a.at(col, row) != kShadow && a.at(col, row) != kShadow2) {
                s0 = std::min(s0, col);
                s1 = std::max(s1, col);
                sBottom = std::max(sBottom, row);
            }
        }
    }
    if (x1 < 0) {
        return;
    }
    if (building) {
        if (s1 < 0) {
            return;
        }
        x = kBuildingCentre - (s1 - s0 + 1) / 2 - s0;
        y = kBuildingBottom - sBottom;
    }
    width_ = x1 - x0 + 1;
    height_ = y1 - y0 + 1;
    pixels_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));
    drawn_.resize(pixels_.size());
    for (int row = 0; row < height_; ++row) {
        for (int col = 0; col < width_; ++col) {
            const std::size_t i = static_cast<std::size_t>(row) * static_cast<std::size_t>(width_) +
                                  static_cast<std::size_t>(col);
            pixels_[i] = a.at(x0 + col, y0 + row);
            drawn_[i] = a.at(x0 + col, y0 + row) == b.at(x0 + col, y0 + row);
        }
    }
    at_[0] = std::array<int, 2>{x + x0, y + y0};
    if (colony) {
        at_[1] = std::array<int, 2>{x + x0 + (building ? kBuildingColonyDx : kColonyDx),
                                    y + y0 + (building ? kBuildingColonyDy : kColonyDy)};
    }
}

std::optional<std::array<int, 2>> ProductPicture::position(Box box) const {
    return at_[box == Box::BuildList ? 0 : 1];
}

void ProductPicture::draw(Canvas& canvas, Box box, const Palette& palette) const {
    const auto at = position(box);
    if (empty() || !at) {
        return;
    }
    const Rect clip = box == Box::BuildList ? kBuildListClip : kColonyClip;
    // Shadow: what is under it in the colour nearest half as bright, as on
    // the landscape.
    const auto shade = [&](std::uint8_t under) {
        int best = -1;
        std::uint8_t found = under;
        for (std::size_t j = 0; j < 256; ++j) {
            int d = 0;
            for (int ch = 0; ch < 3; ++ch) {
                const int e = six(palette.colors[j], ch) - six(palette.colors[under], ch) / 2;
                d += e * e;
            }
            if (best < 0 || d < best) {
                best = d;
                found = static_cast<std::uint8_t>(j);
            }
        }
        return found;
    };
    ColourMap colours{};
    for (std::size_t i = 0; i < colours.size(); ++i) {
        colours[i] = static_cast<std::uint8_t>(i);
    }
    if (own_) {
        colours = nearestColours(*own_, palette);
        // Measured (colour 224): the palette's unused colours, pure green,
        // show in the screen's own.
        for (std::size_t i = 0; i < colours.size(); ++i) {
            if (own_->colors[i] == Color{0, 255, 0}) {
                colours[i] = static_cast<std::uint8_t>(i);
            }
        }
    }
    for (int row = 0; row < height_; ++row) {
        for (int col = 0; col < width_; ++col) {
            const std::size_t i = static_cast<std::size_t>(row) * static_cast<std::size_t>(width_) +
                                  static_cast<std::size_t>(col);
            const int cx = (*at)[0] + col, cy = (*at)[1] + row;
            if (!drawn_[i] || !clip.contains(cx, cy) || cx >= canvas.width() || cy >= canvas.height()) {
                continue;
            }
            const std::uint8_t v = pixels_[i];
            canvas.set(cx, cy, !own_ && (v == kShadow || v == kShadow2) ? shade(canvas.at(cx, cy)) : colours[v]);
        }
    }
}

} // namespace hegemon
