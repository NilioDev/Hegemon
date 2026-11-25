// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/colonies_screen.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/ui/build_list.hpp"

#include <algorithm>
#include <cstdlib>

namespace hegemon {

namespace {

// Measured on the original; see docs/rules/colonies-screen.md.
constexpr const char* kArchive = "COLSUM.LBX";
constexpr std::size_t kBackgroundEntry = 0, kReturnEntry = 1, kUpEntry = 2, kDownEntry = 3, kFirstSortEntry = 4,
                      kFirstBuyEntry = 11;
struct Placed {
    int x, y;
};
constexpr std::array<Placed, ColoniesScreen::kSorts> kSortAt = {
    {{89, 446}, {140, 446}, {219, 446}, {262, 446}, {326, 446}, {393, 446}, {480, 446}}};
constexpr std::array<int, ColoniesScreen::kSorts> kSortWidth = {50, 78, 42, 63, 66, 86, 36};
constexpr int kSortHeight = 24;
// The rows: one every 31 pixels from y 35, the lines between them included.
constexpr int kRowTop = 35, kRowStep = 31;
struct Span {
    int x, w;
};
constexpr std::array<Span, ColoniesScreen::kColumns> kColumnAt = {
    {{8, 90}, {98, 135}, {233, 142}, {375, 134}, {509, 90}, {599, 19}}};
// In a row, from its top: the name in font 2 at (12, +8), the people at
// +3 from x 101, 236 and 378, what is built in font 1 at (512, +3), and
// the Buy button at (599, +0).
constexpr int kNameX = 12, kNameY = 8;
constexpr std::array<int, kJobs> kPeopleX = {101, 236, 378};
constexpr int kPeopleY = 3, kBuildingX = 512, kBuildingY = 3, kBuyX = 599;
// People are 30 pixels apart, closer when they don't fit (four farmers were
// 28 apart; the rule is assumed).
constexpr int kPersonStep = 30, kPeopleWidth = 84;
constexpr const char* kPeopleArchive = "RACEICON.LBX";
constexpr std::size_t kPeoplePerRace = 13;
constexpr std::array<std::size_t, kJobs> kPersonEntry = {1, 3, 5};
// People held are drawn by the pointer, as in the colony screen (assumed).
constexpr int kHeldDX = 8, kHeldDY = -10, kHeldStep = 15;
// The details of the colony shown: font 1 at x 13, a line every 11 pixels
// from y 354.
constexpr int kDetailsX = 13, kDetailsY = 354, kDetailsStep = 11;
// What it makes: COLONY2.LBX's smaller icons from entry 27, in the order of
// the colony screen's, from x 106 and 20 apart; the rows of food,
// production, research and money at y 349, 367, 385 and 403. How they
// squeeze together when they don't fit is assumed.
constexpr const char* kIconArchive = "COLONY2.LBX";
constexpr std::size_t kFirstIcon = 27, kIcons = 16;
constexpr int kIconX = 106, kIconStep = 20, kIconWidth = 250;
constexpr std::array<std::pair<ColonyScreen::Row, int>, 4> kIconRows = {
    {{ColonyScreen::Food, 349}, {ColonyScreen::Production, 367}, {ColonyScreen::Research, 385}, {ColonyScreen::Money, 403}}};
// The small map: a star at (380 + x × 128 / width, 349 + y × 91 / height)
// of the galaxy, a cross in colours 135 and 132, a black hole a ring in 124
// and 122 around 120; the shown colony's star in 239 and 238. Its name in
// font 1 centred on 444 at y 431.
constexpr int kMapX = 380, kMapY = 349, kMapWidth = 128, kMapHeight = 91;
constexpr std::uint8_t kStarCentre = 135, kStarArm = 132, kHoleCentre = 120, kHoleArm = 124, kHoleCorner = 122,
                       kShownCentre = 239, kShownArm = 238;
constexpr int kStarNameCentre = 444, kStarNameY = 431;
// The empire's totals: font 2, the words at x 520, the numbers ending at
// 627, a line every 13 pixels from y 354.
constexpr int kTotalsX = 520, kTotalsRight = 627, kTotalsY = 354, kTotalsStep = 13;
// Text colours.
constexpr std::uint8_t kNameShade1 = 244, kNameShade2 = 133, kShade1 = 245, kShade2 = 253;
// ESTRINGS.LBX: the details, one line each; the totals; and the words
// they use.
constexpr std::size_t kDetailsMessage = 74;
constexpr std::array<std::size_t, 6> kTotalMessages = {118, 105, 114, 103, 102, 117};
constexpr std::array<std::size_t, 5> kSizeMessages = {683, 480, 371, 360, 323};
constexpr std::array<std::size_t, 10> kClimateMessages = {539, 719, 720, 721, 722, 399, 501, 184, 723, 303};
constexpr std::array<std::size_t, 5> kMineralMessages = {684, 423, 685, 450, 686};
constexpr std::array<std::size_t, 3> kGravityMessages = {687, 688, 689};
constexpr std::size_t kLastMessage = 723;
constexpr std::size_t kPrimeText = 160; // HESTRNGS.LBX

TextStyle style(std::uint8_t shade1, std::uint8_t shade2, int spacing) {
    TextStyle s;
    s.shades[1] = shade1;
    s.shades[2] = s.shades[3] = shade2;
    s.spacing = spacing;
    return s;
}

int personStep(int n) {
    return n < 2 ? kPersonStep : std::min(kPersonStep, kPeopleWidth / (n - 1));
}

// "%sReserve: %s%d" says "Reserve:".
std::string totalWord(const std::string& message) {
    std::string s = message;
    for (std::size_t at; (at = s.find("%s")) != std::string::npos;) {
        s.erase(at, 2);
    }
    const auto colon = s.find(':');
    return colon == std::string::npos ? s : s.substr(0, colon + 1);
}

} // namespace

Rect ColoniesScreen::sortRect(Sort sort) {
    const auto i = static_cast<std::size_t>(sort);
    return {kSortAt.at(i).x, kSortAt.at(i).y, kSortWidth.at(i), kSortHeight};
}

Rect ColoniesScreen::cellRect(std::size_t row, Column column) {
    const Span& s = kColumnAt.at(static_cast<std::size_t>(column));
    return {s.x, kRowTop + kRowStep * static_cast<int>(row), s.w, kRowStep};
}

ColoniesScreen::ColoniesScreen(GameData& data, const Game& game, const Economy& economy, int player)
    : background_(Image::parse(data.archive(kArchive).entry(kBackgroundEntry))),
      return_(Image::parse(data.archive(kArchive).entry(kReturnEntry))),
      up_(Image::parse(data.archive(kArchive).entry(kUpEntry))),
      down_(Image::parse(data.archive(kArchive).entry(kDownEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    for (std::size_t i = 0; i < kSorts; ++i) {
        sorts_[i] = Image::parse(data.archive(kArchive).entry(kFirstSortEntry + i));
    }
    for (std::size_t i = 0; i < kRowsShown; ++i) {
        buyButtons_[i] = Image::parse(data.archive(kArchive).entry(kFirstBuyEntry + i));
    }
    for (std::size_t i = 0; i < kIcons; ++i) {
        icons_.push_back(Image::parse(data.archive(kIconArchive).entry(kFirstIcon + i)));
    }
    const Player& owner = game.players.at(static_cast<std::size_t>(player));
    const auto race = static_cast<std::size_t>(owner.race);
    for (std::size_t j = 0; j < kJobs; ++j) {
        people_[j] = Image::parse(data.archive(kPeopleArchive).entry(race * kPeoplePerRace + kPersonEntry[j]));
    }
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    if (strings.size() <= kPrimeText || messages.size() <= kLastMessage) {
        throw FormatError("too few messages in HESTRNGS.LBX or ESTRINGS.LBX");
    }
    std::vector<std::string> detailLines;
    {
        const std::string& d = messages[kDetailsMessage];
        std::size_t from = 0;
        for (std::size_t at; (at = d.find('\n', from)) != std::string::npos; from = at + 1) {
            detailLines.push_back(d.substr(from, at - from));
        }
        detailLines.push_back(d.substr(from));
        if (detailLines.size() < 7) {
            throw FormatError("ESTRINGS.LBX: the colony details are missing");
        }
    }

    int population = 0;
    for (std::size_t i = 0; i < game.colonies.size(); ++i) {
        const Colony& c = game.colonies[i];
        if (c.player != player) {
            continue;
        }
        const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
        const ColonyOutput out = economy.output(game, c);
        Row row;
        row.colony = i;
        row.name = planetName(game.galaxy, c.planet, strings[kPrimeText]);
        row.jobs = c.jobs;
        row.population = c.population();
        row.food = out.food2 - out.eaten2;
        row.industry = std::max(0, out.production - out.upkeep);
        row.science = out.research;
        row.money = out.income2;
        row.star = planet.star;
        row.icons = ColonyScreen::iconRows(out);
        population += c.population();
        int cost = 0, turns = 0;
        if (!c.buildList.empty()) {
            const std::string& what = c.buildList.front();
            row.product = productName(data, what, &owner);
            cost = productCost(data.parameters(), what, owner.designs);
            const int left = std::max(0, cost - c.produced);
            // As in the colony screen: with nothing made a turn, as if one were.
            const int perTurn = std::max(1, row.industry);
            turns = std::max(1, (left + perTurn - 1) / perTurn);
            row.building = formatMessage("%s - %dt", {row.product, static_cast<long long>(turns)});
        }
        const auto size = static_cast<std::size_t>(planet.size) % kSizeMessages.size();
        const auto climate = static_cast<std::size_t>(planet.climate) % kClimateMessages.size();
        row.details = {
            formatMessage(detailLines[0], {std::string(), messages[kSizeMessages[size]],
                                           messages[kClimateMessages[climate]]}),
            formatMessage(detailLines[1],
                          {messages[kGravityMessages[static_cast<std::size_t>(planet.gravity) % kGravityMessages.size()]]}),
            formatMessage(detailLines[2],
                          {messages[kMineralMessages[static_cast<std::size_t>(planet.minerals) % kMineralMessages.size()]]}),
            formatMessage(detailLines[3], {static_cast<long long>(c.population()),
                                           static_cast<long long>(economy.maxPopulation(owner, planet))}),
            // Hegemon sends no people yet.
            formatMessage(detailLines[4], {std::string(), 0LL}),
            // The last number is assumed to be 0 until its meaning is measured.
            formatMessage(detailLines[5], {static_cast<long long>(c.growth),
                                           static_cast<long long>(economy.growth(game, c)), 0LL}),
            formatMessage(detailLines[6], {static_cast<long long>(cost == 0 ? 0 : c.produced),
                                           static_cast<long long>(cost), static_cast<long long>(row.industry)}),
        };
        for (std::string& line : row.details) {
            while (!line.empty() && line.back() == ' ') {
                line.pop_back();
            }
        }
        rows_.push_back(std::move(row));
    }

    const EmpireSummary sum = economy.summary(game, player);
    const std::array<std::string, 6> values = {
        std::to_string(sum.money),
        formatMessage("%+d", {static_cast<long long>(sum.income)}),
        std::to_string(population),
        std::to_string(sum.freighters),
        formatMessage("%+d", {static_cast<long long>(sum.food)}),
        std::to_string(sum.research),
    };
    for (std::size_t i = 0; i < kTotalMessages.size(); ++i) {
        totals_.emplace_back(totalWord(messages[kTotalMessages[i]]), values[i]);
    }

    const Galaxy& galaxy = game.galaxy;
    const int width = std::max(1, galaxy.width), height = std::max(1, galaxy.height);
    for (const Star& star : galaxy.stars) {
        stars_.push_back({kMapX + star.x * kMapWidth / width, kMapY + star.y * kMapHeight / height,
                          star.colour == StarColour::BlackHole});
        starNames_.push_back(star.name);
    }
    sortRows();
}

void ColoniesScreen::sortRows() {
    order_.resize(rows_.size());
    for (std::size_t i = 0; i < order_.size(); ++i) {
        order_[i] = i;
    }
    // Assumed until measured: names in alphabetical order, numbers from the
    // largest; equal rows in the order the colonies were founded.
    const auto key = [this](std::size_t i) {
        const Row& r = rows_[i];
        switch (sort_) {
        case Sort::Population:
            return r.population;
        case Sort::Food:
            return r.food;
        case Sort::Industry:
            return r.industry;
        case Sort::Science:
            return r.science;
        case Sort::Money:
            return r.money;
        default:
            return 0;
        }
    };
    std::stable_sort(order_.begin(), order_.end(), [&](std::size_t a, std::size_t b) {
        if (sort_ == Sort::Name) {
            return rows_[a].name < rows_[b].name;
        }
        if (sort_ == Sort::Producing) {
            return rows_[a].product < rows_[b].product;
        }
        return key(a) > key(b);
    });
    colonies_.clear();
    for (const std::size_t i : order_) {
        colonies_.push_back(rows_[i].colony);
    }
    if (!order_.empty() && shown_ >= rows_.size()) {
        shown_ = order_.front();
    }
}

void ColoniesScreen::setSort(Sort sort) {
    sort_ = sort;
    sortRows();
}

void ColoniesScreen::scroll(int rows) {
    const int most = std::max(0, static_cast<int>(rows_.size()) - static_cast<int>(kRowsShown));
    top_ = static_cast<std::size_t>(std::clamp(static_cast<int>(top_) + rows, 0, most));
}

std::optional<std::size_t> ColoniesScreen::shown() const {
    if (rows_.empty()) {
        return std::nullopt;
    }
    return rows_[shown_].colony;
}

const ColoniesScreen::Row* ColoniesScreen::rowAt(std::size_t visible) const {
    const std::size_t i = top_ + visible;
    return visible < kRowsShown && i < order_.size() ? &rows_[order_[i]] : nullptr;
}

std::optional<std::size_t> ColoniesScreen::visibleRowAt(int y) const {
    if (y < kRowTop) {
        return std::nullopt;
    }
    const auto r = static_cast<std::size_t>((y - kRowTop) / kRowStep);
    return r < kRowsShown ? std::optional<std::size_t>(r) : std::nullopt;
}

void ColoniesScreen::showRow(std::size_t visible) {
    if (const Row* row = rowAt(visible)) {
        shown_ = static_cast<std::size_t>(row - rows_.data());
    }
}

const std::vector<std::string>& ColoniesScreen::details() const {
    static const std::vector<std::string> none;
    return rows_.empty() ? none : rows_[shown_].details;
}

const std::string& ColoniesScreen::name(std::size_t colony) const {
    for (const Row& r : rows_) {
        if (r.colony == colony) {
            return r.name;
        }
    }
    throw std::out_of_range("not a colony of the screen");
}

const std::string& ColoniesScreen::building(std::size_t colony) const {
    for (const Row& r : rows_) {
        if (r.colony == colony) {
            return r.building;
        }
    }
    throw std::out_of_range("not a colony of the screen");
}

const std::array<int, kJobs>& ColoniesScreen::jobs(std::size_t colony) const {
    for (const Row& r : rows_) {
        if (r.colony == colony) {
            return r.jobs;
        }
    }
    throw std::out_of_range("not a colony of the screen");
}

void ColoniesScreen::pointerMoved(int x, int y) {
    pointerX_ = x;
    pointerY_ = y;
    // Assumed: the details are of the colony last under the pointer.
    if (x >= kColumnAt[0].x && x < kColumnAt[kColumns - 1].x + kColumnAt[kColumns - 1].w) {
        if (const auto r = visibleRowAt(y)) {
            showRow(*r);
        }
    }
}

void ColoniesScreen::press(int x, int y) {
    pointerMoved(x, y);
    held_ = Rect{};
    heldCount_ = 0;
    const auto r = visibleRowAt(y);
    if (r) {
        if (const Row* row = rowAt(*r)) {
            for (std::size_t j = 0; j < kJobs; ++j) {
                const Rect cell = cellRect(*r, static_cast<Column>(j + 1));
                const int n = row->jobs[j];
                const int k = (x - kPeopleX[j]) / personStep(n);
                if (cell.contains(x, y) && x >= kPeopleX[j] && k < n) {
                    // Measured: the person pressed is picked up with all to
                    // its right.
                    heldRow_ = static_cast<std::size_t>(row - rows_.data());
                    heldJob_ = j;
                    heldCount_ = n - k;
                    return;
                }
            }
        }
    }
    std::vector<Rect> buttons = {kReturnButton, kUpButton, kDownButton};
    for (std::size_t s = 0; s < kSorts; ++s) {
        buttons.push_back(sortRect(static_cast<Sort>(s)));
    }
    if (r && rowAt(*r)) {
        for (const Column c : {Column::Name, Column::Building, Column::Buy}) {
            buttons.push_back(cellRect(*r, c));
        }
    }
    for (const Rect& b : buttons) {
        if (b.contains(x, y)) {
            held_ = b;
        }
    }
}

ColoniesScreen::Action ColoniesScreen::release(int x, int y) {
    pointerMoved(x, y);
    if (heldCount_ > 0) {
        // Assumed, as in the colony screen: let go over another job of the
        // same colony, the people move there; anywhere else they go back.
        const int count = heldCount_;
        heldCount_ = 0;
        const auto r = visibleRowAt(y);
        const Row* row = r ? rowAt(*r) : nullptr;
        if (row == nullptr || static_cast<std::size_t>(row - rows_.data()) != heldRow_) {
            return {};
        }
        for (std::size_t j = 0; j < kJobs; ++j) {
            if (j != heldJob_ && cellRect(*r, static_cast<Column>(j + 1)).contains(x, y)) {
                Row& changed = rows_[heldRow_];
                changed.jobs[heldJob_] -= count;
                changed.jobs[j] += count;
                return {Action::Kind::Jobs, changed.colony};
            }
        }
        return {};
    }
    const Rect held = held_;
    held_ = Rect{};
    if (held.w == 0 || !held.contains(x, y)) {
        return {};
    }
    if (held.x == kReturnButton.x && held.y == kReturnButton.y) {
        return {Action::Kind::Return, 0};
    }
    if (held.x == kUpButton.x) {
        scroll(held.y == kUpButton.y ? -1 : 1);
        return {};
    }
    for (std::size_t s = 0; s < kSorts; ++s) {
        if (held.x == kSortAt[s].x && held.y == kSortAt[s].y) {
            setSort(static_cast<Sort>(s));
            return {};
        }
    }
    const auto r = visibleRowAt(held.y + 1);
    const Row* row = r ? rowAt(*r) : nullptr;
    if (row == nullptr) {
        return {};
    }
    // Measured: the name opens the colony screen, what is built the build
    // list, and the round button buys it.
    if (held.x == kColumnAt[static_cast<std::size_t>(Column::Name)].x) {
        return {Action::Kind::Open, row->colony};
    }
    if (held.x == kColumnAt[static_cast<std::size_t>(Column::Building)].x) {
        return {Action::Kind::Build, row->colony};
    }
    return {Action::Kind::Buy, row->colony};
}

std::pair<int, int> ColoniesScreen::step(int x, int y, int dx, int dy) const {
    // The lines of places: each row with a colony, then the buttons below.
    std::vector<std::vector<Rect>> lines;
    for (std::size_t r = 0; r < kRowsShown && rowAt(r); ++r) {
        std::vector<Rect> line;
        for (std::size_t c = 0; c < kColumns; ++c) {
            line.push_back(cellRect(r, static_cast<Column>(c)));
        }
        lines.push_back(std::move(line));
    }
    std::vector<Rect> bottom;
    for (std::size_t s = 0; s < kSorts; ++s) {
        bottom.push_back(sortRect(static_cast<Sort>(s)));
    }
    bottom.push_back(kReturnButton);
    lines.push_back(std::move(bottom));
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
    const int cx = lines[line][at].centerX();
    if (dy != 0) {
        line = static_cast<std::size_t>(std::clamp(static_cast<int>(line) + dy, 0, static_cast<int>(lines.size()) - 1));
        at = 0;
        for (std::size_t i = 1; i < lines[line].size(); ++i) {
            if (std::abs(lines[line][i].centerX() - cx) < std::abs(lines[line][at].centerX() - cx)) {
                at = i;
            }
        }
    }
    if (dx != 0) {
        at = static_cast<std::size_t>(
            std::clamp(static_cast<int>(at) + dx, 0, static_cast<int>(lines[line].size()) - 1));
    }
    return {lines[line][at].centerX(), lines[line][at].centerY()};
}

void ColoniesScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    // The small map's part of the picture is see-through: black under it.
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            canvas.set(x, y, 0);
        }
    }
    background_.draw(0, canvas);
    const auto pressed = [this](const Rect& r) { return held_.w > 0 && held_.x == r.x && held_.y == r.y; };
    // Measured: Return is drawn over the background's darker one.
    return_.draw(pressed(kReturnButton) ? 1 : 0, canvas, kReturnButton.x, kReturnButton.y);
    if (pressed(kUpButton)) {
        up_.draw(1, canvas, kUpButton.x, kUpButton.y);
    }
    if (pressed(kDownButton)) {
        down_.draw(1, canvas, kDownButton.x, kDownButton.y);
    }
    // The button of the order chosen is lit, the others as the background
    // shows them.
    const auto chosen = static_cast<std::size_t>(sort_);
    sorts_[chosen].draw(1, canvas, kSortAt[chosen].x, kSortAt[chosen].y);

    const Font& nameFont = fonts_.font(2);
    const Font& smallFont = fonts_.font(1);
    const TextStyle smallStyle = style(kShade1, kShade2, 1);
    TextStyle nameStyle = style(kNameShade1, kNameShade2, 2);
    // Measured: the names and the star's have a black shadow.
    nameStyle.shadow = std::uint8_t{0};
    nameStyle.shadowAt = {{1, 0}, {0, 1}, {1, 1}};
    TextStyle starStyle = smallStyle;
    starStyle.shadow = nameStyle.shadow;
    starStyle.shadowAt = nameStyle.shadowAt;
    for (std::size_t r = 0; r < kRowsShown; ++r) {
        const Row* row = rowAt(r);
        if (row == nullptr) {
            break;
        }
        const int top = kRowTop + kRowStep * static_cast<int>(r);
        nameFont.draw(canvas, kNameX, top + kNameY, row->name, nameStyle);
        const bool holding = heldCount_ > 0 && static_cast<std::size_t>(row - rows_.data()) == heldRow_;
        for (std::size_t j = 0; j < kJobs; ++j) {
            const int n = row->jobs[j] - (holding && j == heldJob_ ? heldCount_ : 0);
            const int step = personStep(row->jobs[j]);
            for (int k = 0; k < n; ++k) {
                people_[j].draw(0, canvas, kPeopleX[j] + step * k, top + kPeopleY);
            }
        }
        smallFont.draw(canvas, kBuildingX, top + kBuildingY, row->building, smallStyle);
        if (pressed(cellRect(r, Column::Buy))) {
            buyButtons_[r].draw(1, canvas, kBuyX, top);
        }
    }

    if (!rows_.empty()) {
        const Row& shown = rows_[shown_];
        for (std::size_t i = 0; i < shown.details.size(); ++i) {
            smallFont.draw(canvas, kDetailsX, kDetailsY + kDetailsStep * static_cast<int>(i), shown.details[i],
                           smallStyle);
        }
        for (const auto& [which, y] : kIconRows) {
            const auto& icons = shown.icons[which];
            const int step = icons.size() < 2 ? kIconStep
                                              : std::min(kIconStep, kIconWidth / static_cast<int>(icons.size() - 1));
            for (std::size_t i = 0; i < icons.size(); ++i) {
                if (icons[i] != ColonyScreen::kGap && static_cast<std::size_t>(icons[i]) < icons_.size()) {
                    icons_[static_cast<std::size_t>(icons[i])].draw(0, canvas, kIconX + step * static_cast<int>(i), y);
                }
            }
        }
    }

    const auto dot = [&canvas](int x, int y, std::uint8_t colour) {
        if (x >= 0 && y >= 0 && x < canvas.width() && y < canvas.height()) {
            canvas.set(x, y, colour);
        }
    };
    const std::size_t shownStar = rows_.empty() ? stars_.size() : static_cast<std::size_t>(rows_[shown_].star);
    for (std::size_t s = 0; s < stars_.size(); ++s) {
        const Dot& d = stars_[s];
        if (d.blackHole) {
            for (const int cx : {-1, 1}) {
                for (const int cy : {-1, 1}) {
                    dot(d.x + cx, d.y + cy, kHoleCorner);
                }
            }
            dot(d.x, d.y, kHoleCentre);
            dot(d.x - 1, d.y, kHoleArm);
            dot(d.x + 1, d.y, kHoleArm);
            dot(d.x, d.y - 1, kHoleArm);
            dot(d.x, d.y + 1, kHoleArm);
            continue;
        }
        const bool here = s == shownStar;
        dot(d.x, d.y, here ? kShownCentre : kStarCentre);
        for (const auto& [ax, ay] : {std::pair{-1, 0}, std::pair{1, 0}, std::pair{0, -1}, std::pair{0, 1}}) {
            dot(d.x + ax, d.y + ay, here ? kShownArm : kStarArm);
        }
    }
    if (shownStar < starNames_.size()) {
        const std::string& n = starNames_[shownStar];
        smallFont.draw(canvas, kStarNameCentre - smallFont.textWidth(n, 1) / 2, kStarNameY, n, starStyle);
    }

    const TextStyle totalStyle = style(kShade1, kShade2, 2);
    for (std::size_t i = 0; i < totals_.size(); ++i) {
        const int y = kTotalsY + kTotalsStep * static_cast<int>(i);
        nameFont.draw(canvas, kTotalsX, y, totals_[i].first, totalStyle);
        nameFont.draw(canvas, kTotalsRight - nameFont.textWidth(totals_[i].second, 2), y, totals_[i].second,
                      totalStyle);
    }

    if (heldCount_ > 0) {
        for (int k = 0; k < heldCount_; ++k) {
            people_[heldJob_].draw(0, canvas, pointerX_ + kHeldDX + kHeldStep * k, pointerY_ + kHeldDY);
        }
    }
}

} // namespace hegemon
