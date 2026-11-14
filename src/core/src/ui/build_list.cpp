// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/build_list.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/ship_design.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/colony_screen.hpp"

#include <algorithm>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colony-screen.md).
constexpr const char* kArchive = "COLBLDG.LBX";
constexpr std::size_t kFrameEntry = 0, kCancelEntry = 1, kOkEntry = 3, kRefitEntry = 4, kDesignEntry = 5;
// Refit and Design, greyed while there is nothing to refit (measured at the
// start of a game).
constexpr int kRefitX = 492, kDesignX = 561, kRefitY = 379;
// ESTRINGS.LBX messages.
constexpr std::size_t kDeleteMessage = 257;   // "delete %s"
constexpr std::size_t kTitleMessage = 210;    // "Build List for %s"
constexpr std::size_t kTimeMessage = 211;     // "Build Time: %d"
constexpr std::size_t kCostMessage = 242;     // "Cost Here: %d"
constexpr std::size_t kMaintenanceMessage = 367; // "Maint. Cost: %d"
constexpr std::size_t kFreightersMessage = 301, kHousingMessage = 322, kSpyMessage = 492, kTradeGoodsMessage = 541,
                      kTransportMessage = 542;
constexpr std::size_t kLastMessage = 542;
// HESTRNGS.LBX: the word for a star's first planet.
constexpr std::size_t kPrimeText = 160;
// The lists in font 4, 19 pixels a row; the build list and the title in
// font 3 centred on 332, 20 pixels a row below the title. Two pixels
// between letters.
constexpr std::size_t kListFont = 4, kQueueFont = 3;
constexpr int kSpacing = 2;
constexpr int kLeftX = 13, kRightX = 485, kListY = 20, kRowStep = 19;
constexpr Rect kLeftArea = {8, 19, 180, 19 * 22};
constexpr Rect kRightArea = {480, 19, 152, 19 * static_cast<int>(BuildListScreen::kRightRows)};
constexpr int kQueueCentre = 332, kTitleY = 311, kQueueY = 335, kQueueStep = 20;
// The frame drawn round the line of the build list the pointer is on, in
// colour 239.
constexpr int kQueueBoxX = 207, kQueueBoxY = 332, kQueueBoxW = 252, kQueueBoxH = 16;
constexpr std::uint8_t kQueueBoxColour = 239;
// The darker boxes behind the title and the lines.
constexpr Rect kTitleBox = {231, 307, 202, 21};
constexpr int kLineBoxX = 206, kLineBoxY = 331, kLineBoxW = 255, kLineBoxH = 17;
// The box at the top: the name in font 4, the cost and the time in font 3.
// A building's or the colony base's maintenance goes at 60 and the time a
// line lower.
constexpr int kInfoX = 306, kInfoNameY = 21, kInfoCostY = 47, kInfoTimeY = 60, kInfoStep = 13;
// The description in the box below, measured: the product's tech's help
// text (HELP.LBX, from byte 103 of its record) in font 3 with one pixel
// between letters, from (209, 119), 15 pixels a line. Words go on a line
// while it is at most 249 pixels wide with 3 between words; every line but
// the last is then spread to 248 by widening the gaps, the leftmost ones a
// pixel more where it doesn't share out evenly.
constexpr const char* kHelpArchive = "HELP.LBX";
constexpr std::size_t kHelpTextAt = 103;
constexpr std::size_t kDescriptionFont = 3;
constexpr int kDescriptionSpacing = 1, kDescriptionX = 209, kDescriptionY = 119, kDescriptionStep = 15;
constexpr int kDescriptionWrap = 249, kDescriptionWidth = 248, kWordGap = 3;
// The right list: the ships and spies at their rows, the designs on rows 6
// to 11.
constexpr std::size_t kSpyRow = 13, kFirstDesignRow = 6;
// A design's box below, measured: font 1 with one pixel between letters,
// 11 pixels a line from y 114; the labels at x 212 and their values at
// 336; the weapons and specials two to a line at 217 and 336.
constexpr std::size_t kDesignFont = 1;
constexpr int kDesignSpacing = 1, kDesignLabelX = 212, kDesignValueX = 336, kDesignItemX = 217, kDesignY = 114,
              kDesignStep = 11;
// ESTRINGS.LBX labels of the box.
constexpr std::size_t kArmorLabel = 91, kComputerLabel = 98, kCostLabel = 99, kShieldLabel = 119,
                      kSpecialsLabel = 129, kWeaponsLabel = 130;
// The original's label for the drive is not in the files.
constexpr const char* kDriveLabel = "Drive type:";
// The Scout's name: KENTEXT1.LBX entry 24. The other designs' names:
// SHIPNAME.LBX entry 0, 48 a race, 8 a hull size.
constexpr std::size_t kScoutNameEntry = 24, kShipNamesPerRace = 48;
// Behind the frame the colony's scene in the blue ramp 244 to 254, as
// measured: a landscape colour (0 to 79) by its brightness in six bits a
// channel, 2 red + green + blue on even rows and 2 red + 2 green + blue on
// the darker odd rows; the sky and the buildings flat. The steps between
// levels lie where the measured colours left room for them.
constexpr std::uint8_t kRamp = 244;
constexpr std::size_t kLandscapeColours = 80;
constexpr std::array<int, 5> kEvenSteps = {28, 57, 83, 116, 150};
constexpr std::array<int, 6> kOddSteps = {33, 66, 96, 127, 158, 190};
constexpr int kEvenFirst = 2, kOddFirst = 0;

int six(std::uint8_t v);

int level(const Color& c, bool odd) {
    const int r = six(c.r), g = six(c.g), b = six(c.b);
    int l = odd ? kOddFirst : kEvenFirst;
    if (odd) {
        for (const int step : kOddSteps) {
            l += 2 * r + 2 * g + b >= step ? 1 : 0;
        }
    } else {
        for (const int step : kEvenSteps) {
            l += 2 * r + g + b >= step ? 1 : 0;
        }
    }
    return l;
}

int six(std::uint8_t v) {
    return (v * 63 + 127) / 255;
}

// The tech whose help text describes a product: the ships' and the colony
// base's own, the freighters', "transport" and "spy_network"; a building's
// tech_id. 0 for housing and trade goods.
int productTech(const Parameters& p, std::string_view product) {
    if (product == kTradeGoods || product == kHousing) {
        return 0;
    }
    if (product == kColonyShip || product == kOutpostShip || product == kColonyBase || product == kFreighters) {
        return static_cast<int>(p.integer("tech", {product}, "id"));
    }
    if (product == kTransportShip) {
        return static_cast<int>(p.integer("tech", {"transport"}, "id"));
    }
    if (product == kSpy) {
        return static_cast<int>(p.integer("tech", {"spy_network"}, "id"));
    }
    return static_cast<int>(p.integer("building", {product}, "tech_id"));
}

std::vector<std::vector<std::string>> wrapDescription(const Font& font, const std::string& text) {
    std::vector<std::vector<std::string>> lines;
    int width = 0;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t end = std::min(text.find(' ', at), text.size());
        std::string word = text.substr(at, end - at);
        at = end + 1;
        if (word.empty()) {
            continue;
        }
        const int w = font.textWidth(word, kDescriptionSpacing);
        if (!lines.empty() && !lines.back().empty() && width + kWordGap + w <= kDescriptionWrap) {
            width += kWordGap + w;
            lines.back().push_back(std::move(word));
        } else {
            width = w;
            lines.push_back({std::move(word)});
        }
    }
    return lines;
}

TextStyle style(const std::array<std::uint8_t, 3>& colours) {
    TextStyle s;
    s.shades[1] = colours[2];
    s.shades[2] = colours[0];
    s.shades[3] = colours[1];
    s.shadow = std::uint8_t{0};
    s.shadowAt = {{1, 0}, {0, 1}, {1, 1}};
    s.spacing = kSpacing;
    return s;
}

// A label of the box: the message without its %s, %d and line ends.
std::string label(const std::string& format) {
    std::string out;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%' && i + 1 < format.size() && (format[i + 1] == 's' || format[i + 1] == 'd')) {
            ++i;
        } else if (format[i] != '\n') {
            out += format[i];
        }
    }
    const auto first = out.find_first_not_of(' ');
    const auto last = out.find_last_not_of(' ');
    return first == std::string::npos ? std::string() : out.substr(first, last - first + 1);
}

// The ship parts' names at the end of TECHNAME.LBX's first entry (counted
// back from its last name, as the buildings before them are more than the
// fan patch's table has): "No Special" and the specials, the armors,
// shields, weapons, computers and drives (each list starting with its
// "No ..."), 4 kinds of units, 6 fuel cells, the mods' long and short names
// (each after a name for none), the weapons' plurals, then the hull sizes'
// names and their plurals.
struct PartNames {
    std::vector<std::string> specials, armor, shields, weapons, computers, drives, shortMods, plurals;
};

PartNames partNames(const Parameters& p, const TechNames& names) {
    const auto rows = [&](std::string_view setting) {
        const ParameterDefinition* d = p.definition(setting);
        if (d == nullptr || d->rowNames.empty()) {
            throw FormatError("missing setting " + std::string(setting));
        }
        return d->rowNames[0].size() + 1;
    };
    constexpr std::size_t kUnits = 4, kFuelCells = 6, kHullNames = 9;
    std::size_t last = 0;
    for (std::size_t i = 0; const std::string* w = names.afterTechs(i); ++i) {
        if (!w->empty()) {
            last = i;
        }
    }
    const std::size_t total = rows("special") + rows("armor") + rows("shield") + rows("weapon") + rows("computer") +
                              rows("drive") + kUnits + kFuelCells + 2 * rows("weapon_mod") + rows("weapon") +
                              2 * kHullNames;
    if (last + 1 < total) {
        throw FormatError("TECHNAME.LBX: too few names");
    }
    std::size_t at = last + 1 - total;
    const auto take = [&](std::size_t n) {
        std::vector<std::string> list;
        for (std::size_t i = 0; i < n; ++i) {
            list.push_back(*names.afterTechs(at + i));
        }
        at += n;
        return list;
    };
    PartNames n;
    n.specials = take(rows("special"));
    n.armor = take(rows("armor"));
    n.shields = take(rows("shield"));
    n.weapons = take(rows("weapon"));
    n.computers = take(rows("computer"));
    n.drives = take(rows("drive"));
    at += kUnits + kFuelCells + rows("weapon_mod");
    n.shortMods = take(rows("weapon_mod"));
    n.plurals = take(rows("weapon"));
    return n;
}

const std::string& nameIn(const std::vector<std::string>& list, int index) {
    static const std::string none;
    return index >= 0 && static_cast<std::size_t>(index) < list.size() ? list[static_cast<std::size_t>(index)] : none;
}

// What the box lists for a design, measured on the original.
std::vector<BuildListScreen::PlacedText> designLines(const ShipDesign& d, const PartNames& n,
                                                     const std::vector<std::string>& messages) {
    std::vector<BuildListScreen::PlacedText> out;
    int y = kDesignY;
    const auto line = [&](const std::string& name, const std::string& value) {
        out.push_back({kDesignLabelX, y, label(name)});
        out.push_back({kDesignValueX, y, value});
        y += kDesignStep;
    };
    line(messages[kCostLabel], std::to_string(d.cost));
    line(messages[kShieldLabel], nameIn(n.shields, d.shield));
    line(messages[kComputerLabel], nameIn(n.computers, d.computer));
    line(messages[kArmorLabel], nameIn(n.armor, d.armor));
    out.push_back({kDesignLabelX, y, kDriveLabel});
    out.push_back({kDesignValueX, y, nameIn(n.drives, d.drive)});
    y += kDesignStep;
    const auto list = [&](const std::string& title, const std::vector<std::string>& items) {
        if (items.empty()) {
            return;
        }
        y += kDesignStep;
        out.push_back({kDesignLabelX, y, label(title)});
        y += kDesignStep;
        for (std::size_t i = 0; i < items.size(); ++i) {
            out.push_back({i % 2 == 0 ? kDesignItemX : kDesignValueX, y, items[i]});
            if (i % 2 == 1 || i + 1 == items.size()) {
                y += kDesignStep;
            }
        }
    };
    std::vector<std::string> weapons;
    for (const WeaponMount& m : d.weapons) {
        std::string text = std::to_string(m.count) + " ";
        for (std::size_t bit = 1; bit < n.shortMods.size(); ++bit) {
            if (((m.mods >> bit) & 1) != 0) {
                text += n.shortMods[bit] + " ";
            }
        }
        text += nameIn(m.count == 1 ? n.weapons : n.plurals, m.weapon);
        weapons.push_back(std::move(text));
    }
    list(messages[kWeaponsLabel], weapons);
    std::vector<std::string> specials;
    for (int id = 1; id < static_cast<int>(n.specials.size()); ++id) {
        if (d.hasSpecial(id)) {
            specials.push_back(n.specials[static_cast<std::size_t>(id)]);
        }
    }
    list(messages[kSpecialsLabel], specials);
    return out;
}

std::array<std::uint8_t, 3> colourGroup(const Parameters& p, std::string_view row) {
    std::array<std::uint8_t, 3> c{};
    const std::array<std::string_view, 3> columns = {"base", "dim", "dark"};
    for (std::size_t i = 0; i < 3; ++i) {
        c[i] = static_cast<std::uint8_t>(p.integer("font_color_group", {"g0", row}, columns[i]) & 0xFF);
    }
    return c;
}

} // namespace

std::string designName(GameData& data, int race, const ShipDesign& design) {
    if (!design.name.empty()) {
        return design.name;
    }
    if (design.nameIndex < 0) {
        const TextTable scout = TextTable::parse(data.archive("KENTEXT1.LBX").entry(kScoutNameEntry));
        if (scout.size() == 0) {
            throw FormatError("KENTEXT1.LBX: the Scout's name is missing");
        }
        return scout.at(0);
    }
    const TextTable names = TextTable::parse(data.archive("SHIPNAME.LBX").entry(0));
    const std::size_t at = kShipNamesPerRace * static_cast<std::size_t>(std::max(race, 0)) +
                           static_cast<std::size_t>(design.nameIndex);
    if (at >= names.size()) {
        throw FormatError("SHIPNAME.LBX: too few names");
    }
    return names.at(at);
}

std::string productName(GameData& data, std::string_view product, const Player* owner) {
    if (const auto slot = designSlot(product)) {
        if (owner == nullptr || *slot >= owner->designs.size()) {
            return {};
        }
        return designName(data, owner->race, owner->designs[*slot]);
    }
    const Parameters& p = data.parameters();
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    if (messages.size() <= kLastMessage) {
        throw FormatError("too few messages in ESTRINGS.LBX");
    }
    if (product == kTradeGoods) {
        return messages[kTradeGoodsMessage];
    }
    if (product == kHousing) {
        return messages[kHousingMessage];
    }
    if (product == kFreighters) {
        return messages[kFreightersMessage];
    }
    if (product == kTransportShip) {
        return messages[kTransportMessage];
    }
    if (product == kSpy) {
        return messages[kSpyMessage];
    }
    const TechTree tree(p);
    const TechNames techs(data.archive("TECHNAME.LBX").entry(0), tree);
    if (product == kColonyShip || product == kOutpostShip || product == kColonyBase) {
        return techs.tech(static_cast<int>(p.integer("tech", {product}, "id")));
    }
    return techs.tech(static_cast<int>(p.integer("building", {product}, "tech_id")));
}

BuildListScreen::BuildListScreen(GameData& data, const Game& game, const Economy& economy, std::size_t colony,
                                 const Canvas& scene, const Palette& colours)
    : colony_(game.colonies.at(colony)), frame_(Image::parse(data.archive(kArchive).entry(kFrameEntry))),
      buttons_{Image::parse(data.archive(kArchive).entry(kCancelEntry)),
               Image::parse(data.archive(kArchive).entry(kOkEntry))},
      refit_(Image::parse(data.archive(kArchive).entry(kRefitEntry))),
      design_(Image::parse(data.archive(kArchive).entry(kDesignEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const Parameters& p = data.parameters();
    const auto pointerColours = data.archive("FONTS.LBX").entry(ColonyScreen::kPointerEntry);
    if (pointerColours.size() < 1024) {
        throw FormatError("FONTS.LBX: the build list's colours are missing");
    }
    base_.set(0, pointerColours.subspan(0, 1024));
    // Measured: a product of the left list is darker until it is in the
    // build list, as the fan patch's colour groups say; the right list,
    // the build list and the box at the top are in the bright one.
    normal_ = colourGroup(p, "normal");
    queued_ = colourGroup(p, "queued");

    for (int y = 0; y < background_.height() && y < scene.height(); ++y) {
        for (int x = 0; x < background_.width() && x < scene.width(); ++x) {
            const std::uint8_t v = scene.at(x, y);
            const bool odd = (y & 1) != 0;
            int l = odd ? kOddFirst : kEvenFirst;
            if (v < kLandscapeColours) {
                l = level(colours.colors[v], odd);
            }
            background_.set(x, y, static_cast<std::uint8_t>(kRamp + l));
        }
    }
    // The title's box and the lines of the build list are darker: each
    // colour the one nearest half as bright, counted in six bits a channel
    // (the lowest index wins a tie), as under a building's shadow.
    Palette p2 = base_;
    frame_.applyPalette(p2);
    std::array<std::uint8_t, 256> shade{};
    for (std::size_t i = 0; i < 256; ++i) {
        const Color& c = p2.colors[i];
        int best = -1;
        for (std::size_t j = 0; j < 256; ++j) {
            const Color& d = p2.colors[j];
            const int er = six(d.r) - six(c.r) / 2, eg = six(d.g) - six(c.g) / 2, eb = six(d.b) - six(c.b) / 2;
            const int e = er * er + eg * eg + eb * eb;
            if (best < 0 || e < best) {
                best = e;
                shade[i] = static_cast<std::uint8_t>(j);
            }
        }
    }
    const auto darken = [&](const Rect& r) {
        for (int y = r.y; y < r.y + r.h; ++y) {
            for (int x = r.x; x < r.x + r.w; ++x) {
                background_.set(x, y, shade[background_.at(x, y)]);
            }
        }
    };
    darken(kTitleBox);
    for (int k = 0; k < static_cast<int>(Colony::kBuildListLength); ++k) {
        darken({kLineBoxX, kLineBoxY + kQueueStep * k, kLineBoxW, kLineBoxH});
    }

    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    if (messages.size() <= kLastMessage) {
        throw FormatError("too few messages in ESTRINGS.LBX");
    }
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings.size() <= kPrimeText) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    const TechTree tree(p);
    const TechNames techs(data.archive("TECHNAME.LBX").entry(0), tree);
    const auto techName = [&](std::string_view tech) {
        return techs.tech(static_cast<int>(p.integer("tech", {tech}, "id")));
    };
    title_ = formatMessage(messages[kTitleMessage], {planetName(game.galaxy, colony_.planet, strings[kPrimeText])});
    deleteFormat_ = messages[kDeleteMessage];
    costFormat_ = messages[kCostMessage];
    timeFormat_ = messages[kTimeMessage];
    maintenanceFormat_ = messages[kMaintenanceMessage];

    const Buildable b = buildable(p, game, colony);
    names_.emplace_back(std::string(kTradeGoods), messages[kTradeGoodsMessage]);
    names_.emplace_back(std::string(kHousing), messages[kHousingMessage]);
    names_.emplace_back(std::string(kFreighters), messages[kFreightersMessage]);
    names_.emplace_back(std::string(kTransportShip), messages[kTransportMessage]);
    names_.emplace_back(std::string(kSpy), messages[kSpyMessage]);
    names_.emplace_back(std::string(kColonyShip), techName(kColonyShip));
    names_.emplace_back(std::string(kOutpostShip), techName(kOutpostShip));
    names_.emplace_back(std::string(kColonyBase), techName(kColonyBase));
    for (const std::string& building : b.buildings) {
        if (building != kTradeGoods && building != kHousing) {
            names_.emplace_back(building, techs.tech(static_cast<int>(p.integer("building", {building}, "tech_id"))));
        }
    }
    // Products of the same order by their names.
    left_ = b.buildings;
    std::stable_sort(left_.begin(), left_.end(), [&](const std::string& x, const std::string& y) {
        const int ox = productOrder(p, x), oy = productOrder(p, y);
        return ox != oy ? ox < oy : name(x) < name(y);
    });
    for (std::size_t i = 0; i < b.others.size(); ++i) {
        if (b.others[i] == kSpy) {
            right_[kSpyRow] = b.others[i];
        } else if (i < kFirstDesignRow) {
            right_[i] = b.others[i];
        }
    }
    const Player& owner = game.players.at(static_cast<std::size_t>(colony_.player));
    const int race = owner.race;
    if (!b.designs.empty()) {
        const PartNames parts = partNames(p, techs);
        for (std::size_t i = 0; i < b.designs.size() && kFirstDesignRow + i < kSpyRow; ++i) {
            const ShipDesign& d = owner.designs.at(i);
            right_[kFirstDesignRow + i] = b.designs[i];
            names_.emplace_back(b.designs[i], designName(data, race, d));
            costs_.emplace_back(b.designs[i], d.cost);
            pictures_.emplace_back(b.designs[i], ProductPicture(data, b.designs[i], race, d.image));
            designTexts_.emplace_back(b.designs[i], designLines(d, parts, messages));
        }
    }
    const TextTable help = TextTable::parse(data.archive(kHelpArchive).entry(0), kHelpTextAt);
    for (const auto& [product, unused] : names_) {
        if (designSlot(product)) {
            continue;
        }
        costs_.emplace_back(product, productCost(p, product));
        pictures_.emplace_back(product, ProductPicture(data, product, race));
        const int tech = productTech(p, product);
        if (tech > 0 && static_cast<std::size_t>(tech) < help.size()) {
            descriptions_.emplace_back(product, help.at(static_cast<std::size_t>(tech)));
        }
        if (product != kTradeGoods && product != kHousing && product != kFreighters && product != kColonyShip &&
            product != kOutpostShip && product != kTransportShip && product != kSpy) {
            maintenance_.emplace_back(product, static_cast<int>(p.integer("building", {product}, "maintenance")));
        }
    }
    const ColonyOutput out = economy.output(game, game.colonies.at(colony));
    production_ = std::max(0, out.production - out.upkeep);
}

const std::string& BuildListScreen::name(const std::string& product) const {
    for (const auto& [key, text] : names_) {
        if (key == product) {
            return text;
        }
    }
    return product;
}

std::size_t BuildListScreen::rows(Column column) const {
    switch (column) {
    case Column::Left:
        return left_.size();
    case Column::Right:
        return kRightRows;
    case Column::Queue:
        return colony_.buildList.size();
    }
    return 0;
}

std::optional<BuildListScreen::Slot> BuildListScreen::slotAt(int x, int y) const {
    if (kLeftArea.contains(x, y)) {
        const auto row = static_cast<std::size_t>((y - kLeftArea.y) / kRowStep);
        if (row < left_.size()) {
            return Slot{Column::Left, row};
        }
    }
    if (kRightArea.contains(x, y)) {
        const auto row = static_cast<std::size_t>((y - kRightArea.y) / kRowStep);
        if (row < kRightRows && !right_[row].empty()) {
            return Slot{Column::Right, row};
        }
    }
    for (std::size_t k = 0; k < colony_.buildList.size(); ++k) {
        const Rect box{kQueueBoxX, kQueueBoxY + kQueueStep * static_cast<int>(k), kQueueBoxW, kQueueBoxH};
        if (box.contains(x, y)) {
            return Slot{Column::Queue, k};
        }
    }
    return std::nullopt;
}

const std::string* BuildListScreen::productAt(const Slot& slot) const {
    switch (slot.column) {
    case Column::Left:
        return slot.row < left_.size() ? &left_[slot.row] : nullptr;
    case Column::Right:
        return slot.row < kRightRows && !right_[slot.row].empty() ? &right_[slot.row] : nullptr;
    case Column::Queue:
        return slot.row < colony_.buildList.size() ? &colony_.buildList[slot.row] : nullptr;
    }
    return nullptr;
}

void BuildListScreen::setHovered(std::optional<Slot> slot) {
    hovered_ = slot;
    hoverName_.clear();
    costText_.clear();
    timeText_.clear();
    maintenanceText_.clear();
    description_.clear();
    designText_.clear();
    const std::string* product = slot ? productAt(*slot) : nullptr;
    if (product == nullptr) {
        hovered_.reset();
        return;
    }
    hoverName_ = name(*product);
    int cost = 0;
    for (const auto& [key, value] : costs_) {
        if (key == *product) {
            cost = value;
        }
    }
    if (cost > 0) {
        costText_ = formatMessage(costFormat_, {static_cast<long long>(cost)});
        // Assumed: with no production the time is left out.
        if (production_ > 0) {
            timeText_ = formatMessage(timeFormat_, {static_cast<long long>((cost + production_ - 1) / production_)});
        }
    }
    for (const auto& [key, value] : maintenance_) {
        if (key == *product) {
            maintenanceText_ = formatMessage(maintenanceFormat_, {static_cast<long long>(value)});
        }
    }
    for (const auto& [key, text] : descriptions_) {
        if (key == *product) {
            description_ = wrapDescription(fonts_.font(kDescriptionFont), text);
        }
    }
    for (const auto& [key, lines] : designTexts_) {
        if (key == *product) {
            designText_ = lines;
        }
    }
}

void BuildListScreen::hover(int x, int y) {
    setHovered(slotAt(x, y));
}

void BuildListScreen::act(const Slot& slot) {
    const std::string* product = productAt(slot);
    if (product == nullptr) {
        return;
    }
    if (slot.column == Column::Queue) {
        colony_.buildList.erase(colony_.buildList.begin() + static_cast<std::ptrdiff_t>(slot.row));
    } else {
        addToBuildList(colony_, *product);
    }
    setHovered(hovered_);
}

void BuildListScreen::press(int x, int y) {
    held_ = slotAt(x, y);
    heldButton_ = kCancelButton.contains(x, y) ? 0 : kOkButton.contains(x, y) ? 1 : -1;
}

BuildListScreen::Result BuildListScreen::release(int x, int y) {
    const int button = heldButton_;
    const auto slot = held_;
    heldButton_ = -1;
    held_.reset();
    if (button == 0 && kCancelButton.contains(x, y)) {
        return Result::Cancel;
    }
    if (button == 1 && kOkButton.contains(x, y)) {
        return Result::Ok;
    }
    if (slot && slotAt(x, y) == slot) {
        act(*slot);
        hover(x, y);
    }
    return Result::None;
}

void BuildListScreen::moveHighlight(int dx, int dy) {
    Slot s = hovered_.value_or(Slot{Column::Left, 0});
    if (hovered_ && dx != 0) {
        static constexpr std::array<Column, 3> order = {Column::Left, Column::Queue, Column::Right};
        auto at = static_cast<int>(std::find(order.begin(), order.end(), s.column) - order.begin());
        for (int tries = 0; tries < 3; ++tries) {
            at = std::clamp(at + (dx > 0 ? 1 : -1), 0, 2);
            if (rows(order[static_cast<std::size_t>(at)]) > 0) {
                s.column = order[static_cast<std::size_t>(at)];
                break;
            }
        }
    }
    if (hovered_ && dy != 0) {
        // The next row with a product, keeping to the column.
        for (std::size_t r = s.row; ;) {
            if ((dy < 0 && r == 0) || (dy > 0 && r + 1 >= rows(s.column))) {
                break;
            }
            r = dy < 0 ? r - 1 : r + 1;
            if (productAt({s.column, r}) != nullptr) {
                s.row = r;
                break;
            }
        }
    }
    // A row the column doesn't have moves to its last.
    while (productAt(s) == nullptr && s.row > 0) {
        --s.row;
    }
    if (productAt(s) == nullptr) {
        s = {Column::Left, 0};
    }
    setHovered(s);
}

void BuildListScreen::choose() {
    if (hovered_) {
        act(*hovered_);
        if (!hovered_ || productAt(*hovered_) == nullptr) {
            moveHighlight(0, 0);
        }
    }
}

void BuildListScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = base_;
    frame_.applyPalette(palette);
    for (int y = 0; y < canvas.height() && y < background_.height(); ++y) {
        for (int x = 0; x < canvas.width() && x < background_.width(); ++x) {
            canvas.set(x, y, background_.at(x, y));
        }
    }
    frame_.draw(0, canvas);
    refit_.draw(0, canvas, kRefitX, kRefitY);
    design_.draw(0, canvas, kDesignX, kRefitY);
    if (heldButton_ >= 0) {
        const Rect r = heldButton_ == 0 ? kCancelButton : kOkButton;
        buttons_[static_cast<std::size_t>(heldButton_)].draw(1, canvas, r.x, r.y);
    }

    const TextStyle normal = style(queued_), dim = style(normal_);
    const auto queued = [&](const std::string& product) {
        return std::find(colony_.buildList.begin(), colony_.buildList.end(), product) != colony_.buildList.end();
    };
    const Font& listFont = fonts_.font(kListFont);
    for (std::size_t k = 0; k < left_.size(); ++k) {
        listFont.draw(canvas, kLeftX, kListY + kRowStep * static_cast<int>(k), name(left_[k]),
                      queued(left_[k]) ? normal : dim);
    }
    for (std::size_t k = 0; k < kRightRows; ++k) {
        if (!right_[k].empty()) {
            listFont.draw(canvas, kRightX, kListY + kRowStep * static_cast<int>(k), name(right_[k]), normal);
        }
    }

    const Font& queueFont = fonts_.font(kQueueFont);
    const auto centred = [&](const std::string& text, int y) {
        queueFont.draw(canvas, kQueueCentre - queueFont.textWidth(text, kSpacing) / 2, y, text, normal);
    };
    centred(title_, kTitleY);
    for (std::size_t k = 0; k < colony_.buildList.size(); ++k) {
        const int y = kQueueY + kQueueStep * static_cast<int>(k);
        std::string text = name(colony_.buildList[k]);
        if (hovered_ && hovered_->column == Column::Queue && hovered_->row == k) {
            text = formatMessage(deleteFormat_, {text});
            const int top = kQueueBoxY + kQueueStep * static_cast<int>(k);
            for (int x = kQueueBoxX; x < kQueueBoxX + kQueueBoxW; ++x) {
                canvas.set(x, top, kQueueBoxColour);
                canvas.set(x, top + kQueueBoxH - 1, kQueueBoxColour);
            }
            for (int yy = top; yy < top + kQueueBoxH; ++yy) {
                canvas.set(kQueueBoxX, yy, kQueueBoxColour);
                canvas.set(kQueueBoxX + kQueueBoxW - 1, yy, kQueueBoxColour);
            }
        }
        centred(text, y);
    }

    if (hovered_) {
        if (const std::string* product = productAt(*hovered_)) {
            for (const auto& [key, picture] : pictures_) {
                if (key == *product) {
                    picture.draw(canvas, ProductPicture::Box::BuildList, palette);
                }
            }
        }
        listFont.draw(canvas, kInfoX, kInfoNameY, hoverName_, normal);
        queueFont.draw(canvas, kInfoX, kInfoCostY, costText_, normal);
        int y = kInfoTimeY;
        if (!maintenanceText_.empty()) {
            queueFont.draw(canvas, kInfoX, y, maintenanceText_, normal);
            y += kInfoStep;
        }
        queueFont.draw(canvas, kInfoX, y, timeText_, normal);

        TextStyle part = normal;
        part.spacing = kDesignSpacing;
        part.shadow.reset();
        part.shadowAt.clear();
        for (const PlacedText& t : designText_) {
            fonts_.font(kDesignFont).draw(canvas, t.x, t.y, t.text, part);
        }

        const Font& font = fonts_.font(kDescriptionFont);
        TextStyle text = normal;
        text.spacing = kDescriptionSpacing;
        for (std::size_t k = 0; k < description_.size(); ++k) {
            const auto& words = description_[k];
            const int gaps = static_cast<int>(words.size()) - 1;
            int extra = 0;
            if (k + 1 < description_.size() && gaps > 0) {
                int width = kWordGap * gaps;
                for (const auto& w : words) {
                    width += font.textWidth(w, kDescriptionSpacing);
                }
                extra = std::max(0, kDescriptionWidth - width);
            }
            int x = kDescriptionX;
            const int ly = kDescriptionY + kDescriptionStep * static_cast<int>(k);
            for (std::size_t i = 0; i < words.size(); ++i) {
                font.draw(canvas, x, ly, words[i], text);
                x += font.textWidth(words[i], kDescriptionSpacing) + kWordGap;
                if (gaps > 0) {
                    x += extra / gaps + (static_cast<int>(i) < extra % gaps ? 1 : 0);
                }
            }
        }
    }
}

} // namespace hegemon
