// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/saved_games.hpp"

#include "hegemon/game_data.hpp"

#include <array>
#include <cstdio>
#include <utility>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/saved-games.md).
// The windows: GAME.LBX 11 (Load) and 14 (Save), drawn for the game's
// colours (FONTS.LBX 1); Load (12), Save (15) and Cancel (13), their second
// frame while held. At (180, 51) over the main menu, (144, 25) over the
// galaxy map; everything else is placed from there.
constexpr const char* kArchive = "GAME.LBX";
constexpr std::size_t kLoadWindow = 11, kSaveWindow = 14, kLoadEntry = 12, kCancelEntry = 13, kSaveEntry = 15;
constexpr std::size_t kGameColours = 1; // of FONTS.LBX
constexpr int kMenuX = 180, kMenuY = 51, kGameX = 144, kGameY = 25;
constexpr Rect kOk = {37, 337, 68, 22}, kOkSave = {37, 338, 68, 21}, kCancel = {171, 338, 68, 21};
// The slots: 29 pixels high, one every 31 from 20 down, 22 in, 232 wide.
constexpr int kSlotX = 22, kSlotY = 20, kSlotPitch = 31, kSlotW = 232, kSlotH = 29;
// In a slot, from the window's corner and the slot's top: the name in font
// 2 at (32, 4); "Stardate:" in font 1 at (32, 18) and the stardate at 74;
// the date centred on 157.5 and the time 4 pixels after it; the player
// picture at (228, 16), GAME.LBX 23 over the main menu, 16 over the map.
// The Save window has the name 4 pixels and the rest 2 pixels further right.
constexpr int kNameX = 32, kNameY = 4, kLineX = 32, kLineY = 18, kStardateX = 74, kDoubleCentre = 315;
constexpr int kDateGap = 4, kPlayerX = 228, kPlayerY = 16, kSaveNameShift = 4, kSaveShift = 2;
constexpr std::size_t kMenuPlayer = 23, kGamePlayer = 16;
constexpr std::size_t kNameFont = 2, kLineFont = 1;
constexpr int kNameSpacing = 2, kLineSpacing = 1;
// The text's colours: over the main menu its own colours 254 and 255, 251
// and 252 for the slot used last; over the map the game's 86 and 91, 89
// and 126.
struct Shades {
    std::uint8_t dark, light, lastDark, lastLight;
};
constexpr Shades kMenuShades = {254, 255, 251, 252}, kGameShades = {86, 91, 89, 126};
constexpr const char* kEmpty = "... empty slot ...";
// The 37 bytes of the saved game's name, the last a zero.
constexpr std::size_t kMaxNameLength = 36;

// The warning: WARNING.LBX 0 at (154, 144), seven frames of lights shown
// back and forth, one every two timer ticks; the text in font 4 centred on
// 322, 88 pixels down, shades 3, 137 and 123 of the screen's colours.
constexpr const char* kWarningArchive = "WARNING.LBX";
constexpr int kWarningX = 154, kWarningY = 144, kWarningCentre = 322, kWarningTextY = 88;
constexpr std::size_t kWarningFont = 4;
constexpr int kWarningSpacing = 2;
constexpr std::uint8_t kWarnShade1 = 3, kWarnShade2 = 137, kWarnShade3 = 123;
// The attack message: lines 17 pixels apart, centred on the warning's line
// (measured), in red (shades fitted to a capture); wrapped to 230
// pixels (assumed: the measured lines need 218 to 240).
constexpr int kAttackLineStep = 17, kAttackWrap = 230;
constexpr std::uint8_t kAttackShade1 = 75, kAttackShade2 = 129, kAttackShade3 = 130;
constexpr std::uint64_t kPitHz = 1193182, kPitDivisor = 65536, kTicksAFrame = 2;

// The game menu: GAME.LBX 0 at (144, 25), its buttons 1 to 6; the volume
// bars are entry 7, as much of it as the volume, at (206, 219) and
// (206, 241).
constexpr std::size_t kGameMenuEntry = 0, kFirstButton = 1, kVolumeEntry = 7;
constexpr int kGameMenuX = 144, kGameMenuY = 25;
constexpr std::array<Rect, GameMenu::kButtons> kGameButtons = {
    Rect{184, 68, 91, 28}, Rect{291, 68, 91, 28}, Rect{184, 113, 91, 28},
    Rect{291, 113, 91, 28}, Rect{184, 332, 91, 27}, Rect{295, 332, 91, 27}};
constexpr int kVolumeX = 206, kMusicY = 219, kSoundY = 241, kVolumeSteps = 10;
// Hegemon plays no sound yet: the bars show the original's first volumes.
constexpr int kMusicVolume = 5, kSoundVolume = 5;

constexpr std::array<const char*, 12> kMonths = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Draws the opaque pixels of a picture's frame left of `width`.
void drawLeft(const Image& image, Canvas& canvas, int x, int y, int width) {
    Canvas a(image.width(), image.height(), 0), b(image.width(), image.height(), 1);
    image.draw(0, a);
    image.draw(0, b);
    for (int row = 0; row < image.height(); ++row) {
        for (int col = 0; col < width && col < image.width(); ++col) {
            const int cx = x + col, cy = y + row;
            if (a.at(col, row) == b.at(col, row) && cx >= 0 && cy >= 0 && cx < canvas.width() &&
                cy < canvas.height()) {
                canvas.set(cx, cy, a.at(col, row));
            }
        }
    }
}

} // namespace

std::string savedDate(int year, int month, int day) {
    const char* name = month >= 1 && month <= 12 ? kMonths[static_cast<std::size_t>(month - 1)] : "???";
    return std::string(name) + " " + std::to_string(day) + ", " + std::to_string(year);
}

std::string savedTime(int hour, int minute) {
    std::array<char, 16> b{};
    std::snprintf(b.data(), b.size(), "%02d:%02d", hour % 100, minute % 100);
    return b.data();
}

SavedGamesWindow::SavedGamesWindow(GameData& data, Kind kind, Over over, Slots slots,
                                   std::optional<std::size_t> last)
    : kind_(kind), over_(over),
      window_(Image::parse(data.archive(kArchive).entry(kind == Kind::Load ? kLoadWindow : kSaveWindow))),
      ok_(Image::parse(data.archive(kArchive).entry(kind == Kind::Load ? kLoadEntry : kSaveEntry))),
      cancel_(Image::parse(data.archive(kArchive).entry(kCancelEntry))),
      player_(Image::parse(data.archive(kArchive).entry(over == Over::MainMenu ? kMenuPlayer : kGamePlayer))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), slots_(std::move(slots)), last_(last) {
    const auto colours = data.archive("FONTS.LBX").entry(kGameColours);
    if (colours.size() < 1024) {
        throw FormatError("FONTS.LBX: the game's colours are missing");
    }
    own_.set(0, colours.subspan(0, 1024));
    if (last_ && *last_ >= kSlots) {
        last_.reset();
    }
}

namespace {

int originX(SavedGamesWindow::Over over) { return over == SavedGamesWindow::Over::MainMenu ? kMenuX : kGameX; }
int originY(SavedGamesWindow::Over over) { return over == SavedGamesWindow::Over::MainMenu ? kMenuY : kGameY; }

} // namespace

Rect SavedGamesWindow::slotRect(std::size_t slot) const {
    return {originX(over_) + kSlotX, originY(over_) + kSlotY + kSlotPitch * static_cast<int>(slot), kSlotW, kSlotH};
}

Rect SavedGamesWindow::okRect() const {
    const Rect r = kind_ == Kind::Load ? kOk : kOkSave;
    return {originX(over_) + r.x, originY(over_) + r.y, r.w, r.h};
}

Rect SavedGamesWindow::cancelRect() const {
    return {originX(over_) + kCancel.x, originY(over_) + kCancel.y, kCancel.w, kCancel.h};
}

SavedGamesWindow::Result SavedGamesWindow::press(int x, int y) {
    held_ = {};
    for (std::size_t s = 0; s < kSlots; ++s) {
        if (!slotRect(s).contains(x, y)) {
            continue;
        }
        if (kind_ == Kind::Load) {
            return {Result::Kind::Load, s, {}};
        }
        // Measured: typing starts from what the slot shows, even the words
        // of an empty one.
        editing_ = s;
        name_ = slots_[s] ? slots_[s]->name : kEmpty;
        if (name_.size() > kMaxNameLength) {
            name_.resize(kMaxNameLength);
        }
        return {};
    }
    if (okRect().contains(x, y)) {
        held_ = okRect();
    } else if (cancelRect().contains(x, y)) {
        held_ = cancelRect();
    }
    return {};
}

SavedGamesWindow::Result SavedGamesWindow::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    if (held.w == 0 || !held.contains(x, y)) {
        return {};
    }
    if (held.x == cancelRect().x) {
        return {Result::Kind::Cancel, 0, {}};
    }
    if (kind_ == Kind::Save) {
        return enter();
    }
    if (last_) {
        return {Result::Kind::Load, *last_, {}};
    }
    return {};
}

bool SavedGamesWindow::type(char c) {
    const auto u = static_cast<unsigned char>(c);
    const Font& font = fonts_.font(kNameFont);
    if (!editing_ || u < 32 || font.width(u) == 0 || name_.size() >= kMaxNameLength ||
        font.textWidth(name_, kNameSpacing) >= kMaxNameWidth) {
        return false;
    }
    name_.push_back(c);
    return true;
}

void SavedGamesWindow::backspace() {
    if (editing_ && !name_.empty()) {
        name_.pop_back();
    }
}

SavedGamesWindow::Result SavedGamesWindow::enter() {
    if (!editing_) {
        return {};
    }
    return {Result::Kind::Save, *editing_, name_};
}

void SavedGamesWindow::tick(std::uint64_t ms) {
    // As the name box: the original shows the cursor every other time it
    // redraws; Hegemon flips it every tick of the PC timer.
    if (timer_.advance(ms) % 2 == 1) {
        cursorShown_ = !cursorShown_;
    }
}

void SavedGamesWindow::draw(Canvas& canvas, const Palette& palette) const {
    const ColourMap colours = nearestColours(own_, palette);
    const int ox = originX(over_), oy = originY(over_);
    window_.draw(0, canvas, ox, oy, colours);
    const Rect ok = okRect(), cancel = cancelRect();
    ok_.draw(held_.w != 0 && held_.x == ok.x ? 1 : 0, canvas, ok.x, ok.y, colours);
    cancel_.draw(held_.w != 0 && held_.x == cancel.x ? 1 : 0, canvas, cancel.x, cancel.y, colours);
    const Shades shades = over_ == Over::MainMenu ? kMenuShades : kGameShades;
    const int nameShift = kind_ == Kind::Save ? kSaveNameShift : 0;
    const int shift = kind_ == Kind::Save ? kSaveShift : 0;
    const Font& nameFont = fonts_.font(kNameFont);
    const Font& lineFont = fonts_.font(kLineFont);
    for (std::size_t s = 0; s < kSlots; ++s) {
        const int top = slotRect(s).y;
        const bool last = last_ == s;
        TextStyle style;
        style.shades[1] = last ? shades.lastDark : shades.dark;
        style.shades[2] = last ? shades.lastLight : shades.light;
        style.spacing = kNameSpacing;
        const std::string& name = editing_ == s ? name_ : slots_[s] ? slots_[s]->name : std::string(kEmpty);
        const int end = nameFont.draw(canvas, ox + kNameX + nameShift, top + kNameY, name, style);
        if (editing_ == s && cursorShown_) {
            nameFont.draw(canvas, end, top + kNameY, "_", style);
        }
        if (!slots_[s]) {
            continue;
        }
        const SaveSlot& slot = *slots_[s];
        style.spacing = kLineSpacing;
        const int line = top + kLineY;
        lineFont.draw(canvas, ox + kLineX + shift, line, "Stardate:", style);
        lineFont.draw(canvas, ox + kStardateX + shift, line,
                      std::to_string(slot.stardate / 10) + "." + std::to_string(slot.stardate % 10), style);
        const int dateWidth = lineFont.textWidth(slot.date, kLineSpacing);
        const int dateX = ox + shift + (kDoubleCentre - dateWidth) / 2;
        lineFont.draw(canvas, dateX, line, slot.date, style);
        lineFont.draw(canvas, dateX + dateWidth + kDateGap, line, slot.time, style);
        player_.draw(0, canvas, ox + kPlayerX, top + kPlayerY);
    }
}

WarningBox::WarningBox(GameData& data, std::string text)
    : box_(Image::parse(data.archive(kWarningArchive).entry(0))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), text_(std::move(text)),
      shades_{kWarnShade1, kWarnShade2, kWarnShade3} {
    if (box_.frameCount() == 0) {
        throw FormatError("WARNING.LBX: the warning box has no pictures");
    }
    lines_.push_back(text_);
}

WarningBox WarningBox::attack(GameData& data, std::string text) {
    WarningBox box(data, std::move(text));
    box.shades_ = {kAttackShade1, kAttackShade2, kAttackShade3};
    box.lines_ = wrapWords(box.fonts_.font(kWarningFont), box.text_, kAttackWrap, kWarningSpacing);
    return box;
}

void WarningBox::advance(std::uint64_t ms) {
    micros_ += ms * 1000;
}

void WarningBox::draw(Canvas& canvas, const Palette& palette) const {
    Palette own = palette;
    box_.applyPalette(own);
    const ColourMap colours = nearestColours(own, palette);
    // Back and forth: 0, 1, ... 6, 5, ... 1, 0, ...
    const std::uint64_t frames = box_.frameCount();
    const std::uint64_t step = micros_ * kPitHz / (kPitDivisor * 1000000) / kTicksAFrame;
    const std::uint64_t period = frames > 1 ? 2 * (frames - 1) : 1;
    const std::uint64_t at = step % period;
    const std::size_t frame = static_cast<std::size_t>(at < frames ? at : period - at);
    box_.draw(frame, canvas, kWarningX, kWarningY, colours);
    TextStyle style;
    style.spacing = kWarningSpacing;
    style.shades[1] = shades_[0];
    style.shades[2] = shades_[1];
    style.shades[3] = shades_[2];
    const Font& font = fonts_.font(kWarningFont);
    const int count = static_cast<int>(lines_.size());
    int y = kWarningY + kWarningTextY - ((count - 1) * kAttackLineStep + 1) / 2;
    for (const std::string& line : lines_) {
        font.draw(canvas, kWarningCentre - font.textWidth(line, kWarningSpacing) / 2, y, line, style);
        y += kAttackLineStep;
    }
}

GameMenu::GameMenu(GameData& data)
    : menu_(Image::parse(data.archive(kArchive).entry(kGameMenuEntry))),
      volume_(Image::parse(data.archive(kArchive).entry(kVolumeEntry))) {
    for (std::size_t b = 0; b < kButtons; ++b) {
        buttons_[b] = Image::parse(data.archive(kArchive).entry(kFirstButton + b));
    }
}

Rect GameMenu::buttonRect(Button b) {
    return kGameButtons[static_cast<std::size_t>(b)];
}

void GameMenu::press(int x, int y) {
    held_.reset();
    for (std::size_t b = 0; b < kButtons; ++b) {
        if (kGameButtons[b].contains(x, y)) {
            held_ = static_cast<Button>(b);
        }
    }
}

std::optional<GameMenu::Button> GameMenu::release(int x, int y) {
    const auto held = std::exchange(held_, std::nullopt);
    if (held && buttonRect(*held).contains(x, y)) {
        return held;
    }
    return std::nullopt;
}

void GameMenu::draw(Canvas& canvas) const {
    menu_.draw(0, canvas, kGameMenuX, kGameMenuY);
    for (std::size_t b = 0; b < kButtons; ++b) {
        const std::size_t frame = held_ && static_cast<std::size_t>(*held_) == b ? 1 : 0;
        buttons_[b].draw(frame < buttons_[b].frameCount() ? frame : 0, canvas, kGameButtons[b].x, kGameButtons[b].y);
    }
    drawLeft(volume_, canvas, kVolumeX, kMusicY, volume_.width() * kMusicVolume / kVolumeSteps);
    drawLeft(volume_, canvas, kVolumeX, kSoundY, volume_.width() * kSoundVolume / kVolumeSteps);
}

} // namespace hegemon
