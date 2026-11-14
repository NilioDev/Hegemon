// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/ui/message_box.hpp"

#include <filesystem>
#include <utility>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Buy: the price measured on the original") {
    // A spy, 100 production: four BC a point with nothing made yet.
    HGM_CHECK(buyPrice(100, 0) == 400);
    // As the original asked for a spy as it was built, 6 a turn.
    const std::vector<std::pair<int, int>> measured = {{6, 340},  {12, 290}, {18, 260}, {24, 230}, {30, 200},
                                                       {36, 170}, {42, 140}, {48, 110}, {54, 92}};
    for (const auto& [made, price] : measured) {
        HGM_CHECK(buyPrice(100, made) == price);
    }
    // And for a colony ship (500).
    HGM_CHECK(buyPrice(500, 0) == 2000 && buyPrice(500, 6) == 1940 && buyPrice(500, 12) == 1880);
    HGM_CHECK(buyPrice(100, 100) == 0 && buyPrice(100, 120) == 0);
}

HGM_TEST("Buy: what Buy does for a colony") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    using Kind = Purchase::Kind;
    Colony c;
    HGM_CHECK(purchase(p, c, 1000, 6).kind == Kind::NothingSelected);
    c.buildList = {"housing", "spy"};
    HGM_CHECK(purchase(p, c, 1000, 6).kind == Kind::NotBuyable);
    c.buildList = {"trade_goods"};
    HGM_CHECK(purchase(p, c, 1000, 6).kind == Kind::NotBuyable);

    c.buildList = {"spy"};
    Purchase offer = purchase(p, c, 58, 6);
    HGM_CHECK(offer.kind == Kind::Short && offer.price == 400 && offer.shortBy == 342 && offer.amount == 100);
    offer = purchase(p, c, 1000, 6);
    HGM_CHECK(offer.kind == Kind::Offer && offer.price == 400 && offer.amount == 100);
    int money = 1000;
    buy(c, money, offer);
    HGM_CHECK(money == 600 && c.produced == 100 && c.boughtPrice == 400);
    // A second Buy takes it back.
    const Purchase undo = purchase(p, c, money, 6);
    HGM_CHECK(undo.kind == Kind::Undo);
    buy(c, money, undo);
    HGM_CHECK(money == 1000 && c.produced == 0 && c.boughtPrice == 0);
    // Nothing to buy when the turn finishes it anyway.
    c.produced = 95;
    HGM_CHECK(purchase(p, c, 1000, 6).kind == Kind::ThisTurn);
}

HGM_TEST("Buy: the message and the question") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    using Lines = std::vector<std::string>;
    MessageBox message(data, MessageBox::Kind::Message,
                       "You cannot afford this increase in production. You are short by 342 BC.");
    HGM_CHECK((message.lines() == Lines{"You cannot afford this increase in production.", "You are short by 342 BC."}));
    // Two lines: the box from 183, Close at (288, 270).
    const Rect close = message.closeRect();
    HGM_CHECK(close.x == 288 && close.y == 270);
    message.press(300, 275);
    HGM_CHECK(message.release(300, 275) == MessageBox::Answer::Close);
    message.press(300, 275);
    HGM_CHECK(message.release(100, 100) == MessageBox::Answer::None);

    MessageBox question(data, MessageBox::Kind::Question, "Do you want to spend 400 BC to buy 100 production?");
    HGM_CHECK((question.lines() == Lines{"Do you want to spend 400", "BC to buy 100 production?"}));
    question.press(260, 314);
    HGM_CHECK(question.release(260, 314) == MessageBox::Answer::Yes);
    question.press(370, 314);
    HGM_CHECK(question.release(370, 314) == MessageBox::Answer::No);

    // Drawn in the screen's colours nearest to the box's own.
    Canvas canvas(640, 480, 7);
    Palette palette;
    for (std::size_t i = 0; i < 256; ++i) {
        const auto v = static_cast<std::uint8_t>(i);
        palette.colors[i] = {v, v, v};
    }
    const Palette before = palette;
    question.draw(canvas, palette);
    HGM_CHECK(palette.colors == before.colors);
    HGM_CHECK(canvas.at(161, 117) == 7 || canvas.at(200, 130) != 7);
    HGM_CHECK(canvas.at(100, 100) == 7);
}
