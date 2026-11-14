// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Hegemon app: opens a window, turns mouse, keyboard and gamepad input
// into commands for the current screen, and shows the 640×480 picture the
// core draws, scaled sharply to the window. For now the screens are the
// main menu, the New Game screen, the race selection, the ruler's name and
// the banner colours.
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "hegemon/data/image.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/banner_select.hpp"
#include "hegemon/ui/main_menu.hpp"
#include "hegemon/ui/new_game.hpp"
#include "hegemon/ui/pointer.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/galaxy_generator.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/build_list.hpp"
#include "hegemon/ui/colony_screen.hpp"
#include "hegemon/ui/galaxy_map.hpp"
#include "hegemon/ui/message_box.hpp"
#include "hegemon/ui/race_select.hpp"
#include "hegemon/ui/research_done.hpp"
#include "hegemon/ui/research_select.hpp"
#include "hegemon/ui/ruler_name.hpp"
#include "hegemon/version.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int kScreenW = 640;
constexpr int kScreenH = 480;
constexpr float kStickThreshold = 0.6f;

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frame = nullptr;
    std::unique_ptr<hegemon::GameData> data;
    std::unique_ptr<hegemon::MainMenu> menu;
    std::unique_ptr<hegemon::Pointer> pointer;
    std::unique_ptr<hegemon::NewGameScreen> newGame; // while it is shown
    std::unique_ptr<hegemon::Races> races;
    std::unique_ptr<hegemon::RaceSelectScreen> raceSelect; // while it is shown
    std::unique_ptr<hegemon::RulerNames> rulerNames;
    std::unique_ptr<hegemon::RulerNameDialog> rulerName; // over the race selection
    std::size_t race = 0;                                  // the stock race taken
    std::unique_ptr<hegemon::BannerSelectScreen> banners;  // after the ruler's name
    std::unique_ptr<hegemon::GeneratingScreen> generating; // after the banner
    std::size_t banner = 0;                                // the banner colour taken
    bool shownGenerating = false; // the message is on the screen: make the galaxy
    std::unique_ptr<hegemon::Game> game;
    std::unique_ptr<hegemon::GalaxyMap> map;
    std::unique_ptr<hegemon::NameBox> homeName; // over the map at first
    std::unique_ptr<hegemon::TechTree> tree;
    std::unique_ptr<hegemon::Random> gameRandom;
    std::unique_ptr<hegemon::ResearchSelectScreen> research; // when the turn ends without a project
    bool researchEndsTurn = false; // choosing there goes on to end the turn
    std::unique_ptr<hegemon::ResearchDoneScreen> researchDone; // a field finished, after the turn
    std::vector<int> learntTechs;                               // still to be shown on it
    std::unique_ptr<hegemon::ColonyScreen> colony;            // over the map
    std::size_t colonyIndex = 0;                              // the colony it shows
    std::unique_ptr<hegemon::BuildListScreen> buildList;      // over the colony screen
    std::unique_ptr<hegemon::MessageBox> message;             // what Buy says, over the colony screen
    hegemon::Purchase purchase;                               // what it offers
    std::mt19937 random{std::random_device{}()};
    hegemon::NewGameSettings settings;
    hegemon::Canvas canvas{kScreenW, kScreenH};
    hegemon::Palette palette;
    std::vector<std::uint8_t> rgba = std::vector<std::uint8_t>(static_cast<std::size_t>(kScreenW) * kScreenH * 4);
    std::vector<std::string> lines;
    int stickDirection = 0; // left stick up (-1) or down (1) while held
    Uint64 lastTicks = 0;
};

// Where a gamedata folder may be: next to the program, in the current
// folder, or in the app's own data folder.
std::optional<std::filesystem::path> findGameFolder() {
    std::vector<std::filesystem::path> candidates;
    if (const char* base = SDL_GetBasePath()) {
        candidates.push_back(std::filesystem::path(base) / "gamedata");
    }
    std::error_code ec;
    candidates.push_back(std::filesystem::current_path(ec) / "gamedata");
    if (char* pref = SDL_GetPrefPath("Hegemon", "Hegemon")) {
        candidates.push_back(std::filesystem::path(pref) / "gamedata");
        SDL_free(pref);
    }
    for (const auto& folder : candidates) {
        if (hegemon::isGameFolder(folder)) {
            return folder;
        }
    }
    return std::nullopt;
}

// Opens the New Game screen with the settings chosen last.
void openNewGame(App& app) {
    try {
        app.newGame = std::make_unique<hegemon::NewGameScreen>(*app.data, app.settings);
        app.pointer->useEntry(hegemon::NewGameScreen::kPointerEntry);
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The New Game screen could not be read: %s", e.what());
    }
}

// What a choice in the main menu does.
SDL_AppResult choose(App& app, hegemon::MainMenu::Choice choice) {
    using Choice = hegemon::MainMenu::Choice;
    if (choice == Choice::Quit) {
        return SDL_APP_SUCCESS;
    }
    if (choice == Choice::NewGame) {
        openNewGame(app);
        return SDL_APP_CONTINUE;
    }
    SDL_Log("This part of the game is not there yet.");
    return SDL_APP_CONTINUE;
}

// Leaves the New Game screen for the main menu.
void finishNewGame(App& app, hegemon::NewGameScreen::Result result) {
    if (result == hegemon::NewGameScreen::Result::Accept) {
        app.settings = app.newGame->settings();
        try {
            if (!app.races) {
                app.races = std::make_unique<hegemon::Races>(*app.data);
            }
            app.raceSelect =
                std::make_unique<hegemon::RaceSelectScreen>(*app.data, *app.races, app.pointer->x(), app.pointer->y());
            app.pointer->useEntry(hegemon::RaceSelectScreen::kPointerEntry);
            app.newGame.reset();
            return;
        } catch (const hegemon::FormatError& e) {
            SDL_Log("The race selection could not be read: %s", e.what());
        }
    }
    app.newGame.reset();
    app.pointer->useEntry(hegemon::Pointer::kMainMenuEntry);
    app.menu->reopen();
    app.menu->pointerMoved(app.pointer->x(), app.pointer->y());
}

// Hegemon's gamepad on the New Game screen: the pointer jumps from item to
// item.
void stepNewGame(App& app, int step) {
    using Item = hegemon::NewGameScreen::Item;
    constexpr int n = static_cast<int>(hegemon::NewGameScreen::kItems);
    const auto current = app.newGame->itemAt(app.pointer->x(), app.pointer->y());
    int at = current ? static_cast<int>(*current) + step : (step > 0 ? 0 : n - 1);
    at = (at + n) % n;
    const auto rect = app.newGame->itemRect(static_cast<Item>(at));
    app.pointer->move(rect.centerX(), rect.centerY());
    app.newGame->pointerMoved(rect.centerX(), rect.centerY());
}

// The mouse position in the 640×480 picture.
void screenPoint(App& app, SDL_Event* event) {
    SDL_ConvertEventToRenderCoordinates(app.renderer, event);
}

} // namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    auto* app = new App();
    *appstate = app;
    SDL_SetAppMetadata("Hegemon", std::string(hegemon::kVersion).c_str(), "io.github.niliodev.hegemon");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Hegemon", kScreenW * 2, kScreenH * 2, SDL_WINDOW_RESIZABLE, &app->window,
                                     &app->renderer)) {
        SDL_Log("Could not open a window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(app->renderer, kScreenW, kScreenH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(app->renderer, 1);
    app->frame =
        SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, kScreenW, kScreenH);
    SDL_SetTextureScaleMode(app->frame, SDL_SCALEMODE_NEAREST);

    std::optional<std::filesystem::path> folder;
    if (argc > 1) {
        folder = std::filesystem::path(argv[1]);
    } else {
        folder = findGameFolder();
    }
    if (folder && hegemon::isGameFolder(*folder)) {
        try {
            app->data = std::make_unique<hegemon::GameData>(*folder);
            if (app->data->config() == nullptr) {
                // Like the patched original, which won't start without its
                // settings.
                app->lines.push_back("Hegemon plays the game with the fan patch 1.51.");
                app->lines.push_back("Install it from moo2mod.com in your game folder");
                app->lines.push_back("and start Hegemon again.");
            } else {
                app->menu = std::make_unique<hegemon::MainMenu>(*app->data);
                app->pointer = std::make_unique<hegemon::Pointer>(*app->data);
            }
        } catch (const hegemon::FormatError& e) {
            app->menu.reset();
            app->lines.push_back(std::string("Your game files could not be read: ") + e.what());
        }
    } else {
        app->lines.push_back("Put your own copy of the original game in a folder");
        app->lines.push_back("named gamedata next to Hegemon and start it again.");
    }
    if (app->menu) {
        SDL_HideCursor(); // the menu draws the original's own pointer
    } else {
        app->lines.push_back("");
        app->lines.push_back("Esc or the gamepad's B button closes this window.");
        for (const auto& line : app->lines) {
            SDL_Log("%s", line.c_str());
        }
    }
    return SDL_APP_CONTINUE;
}

namespace {

SDL_AppResult newGameEvent(App& app, SDL_Event* event) {
    auto& screen = *app.newGame;
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        screen.pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            screen.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (const auto result = screen.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                finishNewGame(app, *result);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) {
            finishNewGame(app, hegemon::NewGameScreen::escape());
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            // The original's Enter clicks where the pointer is.
            if (const auto result = screen.click(app.pointer->x(), app.pointer->y())) {
                finishNewGame(app, *result);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            stepNewGame(app, -1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            stepNewGame(app, 1);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            if (const auto result = screen.click(app.pointer->x(), app.pointer->y())) {
                finishNewGame(app, *result);
            }
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
            finishNewGame(app, hegemon::NewGameScreen::escape());
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY || event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
            const float v = static_cast<float>(event->gaxis.value) / 32767.0f;
            const int direction = v > kStickThreshold ? 1 : v < -kStickThreshold ? -1 : 0;
            if (direction != 0 && direction != app.stickDirection) {
                stepNewGame(app, direction);
            }
            app.stickDirection = direction;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// What choosing a race leads to: the ruler's name, offered one of the
// race's eight at random.
void chooseRace(App& app, std::size_t button) {
    if (button == hegemon::RaceSelectScreen::kCustom) {
        SDL_Log("Designing a custom race is not there yet.");
        return;
    }
    try {
        if (!app.rulerNames) {
            app.rulerNames = std::make_unique<hegemon::RulerNames>(*app.data);
        }
        std::uniform_int_distribution<std::size_t> pick(0, hegemon::RulerNames::kPerRace - 1);
        app.race = button;
        app.rulerName = std::make_unique<hegemon::RulerNameDialog>(*app.data, button,
                                                                   app.rulerNames->name(button, pick(app.random)));
        app.pointer->useEntry(hegemon::RulerNameDialog::kPointerEntry);
        SDL_StartTextInput(app.window);
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The ruler name dialog could not be read: %s", e.what());
    }
}

// Back from the ruler's name to the race selection.
void leaveRulerName(App& app) {
    SDL_StopTextInput(app.window);
    app.rulerName.reset();
}

void acceptRulerName(App& app) {
    try {
        app.banners = std::make_unique<hegemon::BannerSelectScreen>(*app.data, app.race);
        app.pointer->useEntry(hegemon::BannerSelectScreen::kPointerEntry);
        SDL_StopTextInput(app.window);
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The banner colours could not be read: %s", e.what());
    }
}

// Esc on the banner colours: the ruler's name again, with a new name.
void leaveBanners(App& app) {
    app.banners.reset();
    leaveRulerName(app);
    chooseRace(app, app.race);
}

void chooseBanner(App& app, hegemon::BannerSelectScreen::Colour colour) {
    app.banner = static_cast<std::size_t>(colour);
    try {
        app.generating = std::make_unique<hegemon::GeneratingScreen>(*app.data);
        app.shownGenerating = false;
        app.pointer->usePicture(hegemon::Pointer::kBusy);
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The message could not be read: %s", e.what());
    }
}

// Back to the main menu, leaving the new game.
void backToMainMenu(App& app) {
    SDL_StopTextInput(app.window);
    app.homeName.reset();
    app.buildList.reset();
    app.colony.reset();
    app.message.reset();
    app.research.reset();
    app.researchDone.reset();
    app.learntTechs.clear();
    app.map.reset();
    app.game.reset();
    app.generating.reset();
    app.banners.reset();
    app.rulerName.reset();
    app.raceSelect.reset();
    app.pointer->usePicture(hegemon::Pointer::kUsual);
    app.pointer->useEntry(hegemon::Pointer::kMainMenuEntry);
    app.menu->reopen();
    app.menu->pointerMoved(app.pointer->x(), app.pointer->y());
}

// Makes the galaxy for the New Game settings: the human with the race and
// banner taken, the computer players with other stock races and banners at
// random. Then the galaxy map, asking for the home star's name.
// The side panel shows the human's empire as it is now.
void updatePanel(App& app) {
    app.map->setPanel(app.game->stardate, hegemon::Economy(app.data->parameters()).summary(*app.game, 0));
}

void makeGalaxy(App& app) {
    try {
        hegemon::GalaxySettings settings;
        settings.size = app.settings.galaxySize;
        settings.age = app.settings.galaxyAge;
        int players = app.settings.players;
        if (players == hegemon::NewGameSettings::kRandomPlayers) {
            std::uniform_int_distribution<int> pick(hegemon::NewGameSettings::kMinPlayers,
                                                    hegemon::NewGameSettings::kMaxPlayers);
            players = pick(app.random);
        }
        std::vector<std::size_t> races, banners;
        for (std::size_t r = 0; r < hegemon::Races::kStockRaces; ++r) {
            if (r != app.race) {
                races.push_back(r);
            }
        }
        for (std::size_t b = 0; b < hegemon::BannerSelectScreen::kBanners; ++b) {
            if (b != app.banner) {
                banners.push_back(b);
            }
        }
        std::shuffle(races.begin(), races.end(), app.random);
        std::shuffle(banners.begin(), banners.end(), app.random);
        races.insert(races.begin(), app.race);
        banners.insert(banners.begin(), app.banner);
        races.resize(static_cast<std::size_t>(players));
        banners.resize(static_cast<std::size_t>(players));
        for (const std::size_t race : races) {
            settings.players.push_back({static_cast<int>(race), app.races->stock(race)});
        }
        const hegemon::GalaxyGenerator generator(app.data->parameters(), hegemon::StarNames::load(*app.data),
                                                 hegemon::loadNebulae(*app.data));
        hegemon::Random random(app.random());
        std::vector<hegemon::RacePicks> picks;
        for (const auto& p : settings.players) {
            picks.push_back(p.picks);
        }
        app.game = std::make_unique<hegemon::Game>(
            hegemon::startGame(app.data->parameters(), generator.generate(settings, random), picks, banners,
                               random.next()));
        const hegemon::Galaxy& galaxy = app.game->galaxy;
        app.map = std::make_unique<hegemon::GalaxyMap>(*app.data, galaxy, banners);
        app.tree = std::make_unique<hegemon::TechTree>(app.data->parameters());
        app.gameRandom = std::make_unique<hegemon::Random>(app.random());
        for (hegemon::Colony& c : app.game->colonies) {
            c.layout = static_cast<int>(app.gameRandom->below(hegemon::ColonyScreen::kLayouts));
        }
        updatePanel(app);
        app.map->setFleets({galaxy.homes.at(0).star});
        const auto& home = galaxy.stars.at(static_cast<std::size_t>(galaxy.homes.at(0).star));
        app.homeName = std::make_unique<hegemon::NameBox>(hegemon::homeStarNameBox(*app.data, home.name));
        app.generating.reset();
        app.banners.reset();
        app.rulerName.reset();
        app.raceSelect.reset();
        app.pointer->usePicture(hegemon::Pointer::kUsual);
        app.pointer->useEntry(hegemon::GalaxyMap::kPointerEntry);
        SDL_StartTextInput(app.window);
    } catch (const std::exception& e) {
        SDL_Log("The galaxy could not be made: %s", e.what());
        backToMainMenu(app);
    }
}

// While the galaxy is being made, Esc or B goes back to the main menu.
SDL_AppResult generatingEvent(App& app, SDL_Event* event) {
    const bool back = (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) ||
                      (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST);
    if (back) {
        backToMainMenu(app);
    }
    return SDL_APP_CONTINUE;
}

void acceptHomeName(App& app) {
    auto& home = app.game->galaxy.stars.at(static_cast<std::size_t>(app.game->galaxy.homes.at(0).star));
    if (!app.homeName->name().empty()) {
        home.name = app.homeName->name();
    }
    app.homeName.reset();
    SDL_StopTextInput(app.window);
}

void openResearch(App& app, bool endsTurn) {
    const hegemon::Player& human = app.game->players.at(0);
    app.research = std::make_unique<hegemon::ResearchSelectScreen>(*app.data, *app.tree, human.knowledge, human.race,
                                                                    app.pointer->x(), app.pointer->y());
    app.researchEndsTurn = endsTurn;
    app.pointer->useEntry(hegemon::ResearchSelectScreen::kPointerEntry);
}

void afterTurn(App& app);

// The turn ends: first the human chooses a research project if there is
// none, as the original asks.
void endTurn(App& app) {
    hegemon::Player& human = app.game->players.at(0);
    if (human.research == 0) {
        openResearch(app, true);
        return;
    }
    hegemon::endTurn(*app.game, app.data->parameters(), *app.tree, *app.gameRandom);
    updatePanel(app);
    app.learntTechs = human.learntTechs;
    afterTurn(app);
}

// After the turn: a screen for each application learnt, then Select New
// Research when the field is done (measured in that order), then the
// colonies.
void afterTurn(App& app) {
    hegemon::Player& human = app.game->players.at(0);
    app.researchDone.reset();
    while (!app.learntTechs.empty()) {
        const int tech = app.learntTechs.front();
        app.learntTechs.erase(app.learntTechs.begin());
        try {
            app.researchDone =
                std::make_unique<hegemon::ResearchDoneScreen>(*app.data, *app.tree, human.race, human.learntField, tech);
            app.pointer->useEntry(hegemon::ResearchSelectScreen::kPointerEntry);
            return;
        } catch (const std::exception& e) {
            SDL_Log("research done screen: %s", e.what());
        }
    }
    if (human.research == 0 && human.learntField != 0) {
        human.learntField = 0;
        openResearch(app, false);
        return;
    }
    app.pointer->useEntry(hegemon::GalaxyMap::kPointerEntry);
    // Measured: after the first turn the original shows the home colony,
    // which has nothing to build, and not again after the next turn.
    // Assumed until measured further: each colony is shown once.
    for (std::size_t i = 0; i < app.game->colonies.size(); ++i) {
        hegemon::Colony& c = app.game->colonies[i];
        if (c.player == 0 && !c.idleShown) {
            c.idleShown = true;
            app.colony = std::make_unique<hegemon::ColonyScreen>(
                *app.data, *app.game, hegemon::Economy(app.data->parameters()), i);
            app.colonyIndex = i;
            app.pointer->useEntry(hegemon::ColonyScreen::kPointerEntry);
            break;
        }
    }
}

void leaveColony(App& app) {
    app.colony.reset();
    app.pointer->useEntry(hegemon::GalaxyMap::kPointerEntry);
}

void openBuildList(App& app) {
    try {
        app.buildList = std::make_unique<hegemon::BuildListScreen>(
            *app.data, *app.game, hegemon::Economy(app.data->parameters()), app.colonyIndex, app.colony->scene(),
            app.colony->palette());
        app.buildList->hover(app.pointer->x(), app.pointer->y());
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The build list could not be read: %s", e.what());
    }
}

// The colony screen shows what the colony builds now.
void refreshColony(App& app) {
    const std::size_t cursor = app.colony ? app.colony->cursorRow() : 0;
    try {
        app.colony = std::make_unique<hegemon::ColonyScreen>(
            *app.data, *app.game, hegemon::Economy(app.data->parameters()), app.colonyIndex);
        app.colony->moveCursor(static_cast<int>(cursor));
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The colony screen could not be read: %s", e.what());
    }
}

// Leaves the build list; OK keeps what it lists.
void closeBuildList(App& app, bool keep) {
    if (keep) {
        app.game->colonies.at(app.colonyIndex).buildList = app.buildList->buildList();
        refreshColony(app);
    }
    app.buildList.reset();
}

// Buy: asks whether to spend the price, or says why it can't; a second Buy
// in the same turn takes the purchase back.
void startBuy(App& app) {
    hegemon::Colony& c = app.game->colonies.at(app.colonyIndex);
    hegemon::Player& player = app.game->players.at(static_cast<std::size_t>(c.player));
    const hegemon::ColonyOutput out = hegemon::Economy(app.data->parameters()).output(*app.game, c);
    app.purchase =
        hegemon::purchase(app.data->parameters(), c, player.money, std::max(0, out.production - out.upkeep),
                          player.designs);
    using Kind = hegemon::Purchase::Kind;
    if (app.purchase.kind == Kind::Undo) {
        hegemon::buy(c, player.money, app.purchase);
        refreshColony(app);
        return;
    }
    try {
        app.message = std::make_unique<hegemon::MessageBox>(
            *app.data, app.purchase.kind == Kind::Offer ? hegemon::MessageBox::Kind::Question
                                                        : hegemon::MessageBox::Kind::Message,
            app.colony->buyText(app.purchase));
    } catch (const hegemon::FormatError& e) {
        SDL_Log("The message could not be read: %s", e.what());
    }
}

// Closes what Buy says; Yes buys.
void answerBuy(App& app, bool yes) {
    if (yes && app.purchase.kind == hegemon::Purchase::Kind::Offer) {
        hegemon::Colony& c = app.game->colonies.at(app.colonyIndex);
        hegemon::buy(c, app.game->players.at(static_cast<std::size_t>(c.player)).money, app.purchase);
        app.message.reset();
        refreshColony(app);
        return;
    }
    app.message.reset();
}

// What Buy says: a button let go over answers. Enter, Y or the gamepad's A
// say Yes (or Close); Esc, N or B say No (or Close).
SDL_AppResult messageEvent(App& app, SDL_Event* event) {
    hegemon::MessageBox& box = *app.message;
    using Answer = hegemon::MessageBox::Answer;
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            box.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            const Answer a = box.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
            if (a != Answer::None) {
                answerBuy(app, a == Answer::Yes);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_Y:
            answerBuy(app, true);
            break;
        case SDLK_ESCAPE:
        case SDLK_N:
            answerBuy(app, false);
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            answerBuy(app, true);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
            answerBuy(app, false);
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// The build list: a product clicked goes to the end of the list, a line of
// the list clicked is deleted; OK (also Enter, or the gamepad's Start)
// keeps the list, Cancel (Esc, or B) doesn't. The arrow keys or the D-pad
// move a highlight, Space or A adds or deletes what it is on.
SDL_AppResult buildListEvent(App& app, SDL_Event* event) {
    hegemon::BuildListScreen& screen = *app.buildList;
    using Result = hegemon::BuildListScreen::Result;
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        screen.hover(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            screen.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            const Result r = screen.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
            if (r != Result::None) {
                closeBuildList(app, r == Result::Ok);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key) {
        case SDLK_ESCAPE:
            closeBuildList(app, false);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            closeBuildList(app, true);
            break;
        case SDLK_SPACE:
            screen.choose();
            break;
        case SDLK_UP:
        case SDLK_DOWN:
            app.pointer->hide();
            screen.moveHighlight(0, event->key.key == SDLK_UP ? -1 : 1);
            break;
        case SDLK_LEFT:
        case SDLK_RIGHT:
            app.pointer->hide();
            screen.moveHighlight(event->key.key == SDLK_LEFT ? -1 : 1, 0);
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_EAST:
            closeBuildList(app, false);
            break;
        case SDL_GAMEPAD_BUTTON_START:
            closeBuildList(app, true);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            screen.choose();
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            app.pointer->hide();
            screen.moveHighlight(0, event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP ? -1 : 1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            app.pointer->hide();
            screen.moveHighlight(event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : 1, 0);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// The colony screen: Return (also Esc, Enter, or the gamepad's A or B)
// goes back to the map; Change (also C, or the gamepad's X) opens the
// build list; Buy (also B, or the gamepad's Y) buys what is built.
// People moved to other jobs: the colony works on with them.
void moveJobs(App& app) {
    app.game->colonies.at(app.colonyIndex).jobs = app.colony->jobs();
    refreshColony(app);
    updatePanel(app);
}

void colonyAction(App& app, hegemon::ColonyScreen::Action action) {
    switch (action) {
    case hegemon::ColonyScreen::Action::Return:
        leaveColony(app);
        break;
    case hegemon::ColonyScreen::Action::Change:
        openBuildList(app);
        break;
    case hegemon::ColonyScreen::Action::Buy:
        startBuy(app);
        break;
    case hegemon::ColonyScreen::Action::Jobs:
        moveJobs(app);
        break;
    case hegemon::ColonyScreen::Action::None:
        break;
    }
}

void colonyCursor(App& app, int step) {
    app.colony->moveCursor(step);
    const auto [x, y] = app.colony->cursorPoint();
    app.pointer->move(x, y);
    app.pointer->hide();
}

SDL_AppResult colonyEvent(App& app, SDL_Event* event) {
    if (app.buildList) {
        return buildListEvent(app, event);
    }
    if (app.message) {
        return messageEvent(app, event);
    }
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            app.colony->press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app.colony->pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            colonyAction(app,
                         app.colony->release(static_cast<int>(event->button.x), static_cast<int>(event->button.y)));
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE || event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            leaveColony(app);
        } else if (event->key.key == SDLK_C) {
            openBuildList(app);
        } else if (event->key.key == SDLK_B) {
            startBuy(app);
        } else if (event->key.key == SDLK_UP || event->key.key == SDLK_DOWN) {
            colonyCursor(app, event->key.key == SDLK_UP ? -1 : 1);
        } else if (event->key.key == SDLK_SPACE) {
            colonyAction(app, app.colony->pickOrDrop());
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
            leaveColony(app);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            colonyAction(app, app.colony->pickOrDrop());
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP ||
                   event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
            colonyCursor(app, event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP ? -1 : 1);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST) {
            openBuildList(app);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH) {
            startBuy(app);
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void chooseResearch(App& app, int field) {
    hegemon::Player& human = app.game->players.at(0);
    if (field != human.research) {
        human.research = field;
        human.researched = 0;
    }
    human.researchTech = app.research->chosenTech();
    app.research.reset();
    app.pointer->useEntry(hegemon::GalaxyMap::kPointerEntry);
    if (app.researchEndsTurn) {
        endTurn(app);
    } else {
        afterTurn(app);
    }
}

// The research done screen goes at a click, a key or a gamepad button
// (assumed).
SDL_AppResult researchDoneEvent(App& app, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
        if (!event->key.repeat) {
            afterTurn(app);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        afterTurn(app);
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult researchEvent(App& app, SDL_Event* event) {
    auto& screen = *app.research;
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        screen.pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            screen.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (const auto field = screen.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                chooseResearch(app, *field);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            if (const auto field = screen.choose()) {
                chooseResearch(app, *field);
            }
        } else if (event->key.key == SDLK_PAGEUP || event->key.key == SDLK_PAGEDOWN) {
            app.pointer->hide();
            screen.moveApp(event->key.key == SDLK_PAGEUP ? -1 : 1);
        } else if (event->key.key == SDLK_LEFT || event->key.key == SDLK_UP) {
            app.pointer->hide();
            screen.moveHighlight(event->key.key == SDLK_LEFT ? -1 : -2);
        } else if (event->key.key == SDLK_RIGHT || event->key.key == SDLK_DOWN) {
            app.pointer->hide();
            screen.moveHighlight(event->key.key == SDLK_RIGHT ? 1 : 2);
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        app.pointer->hide();
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            screen.moveHighlight(-2);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            screen.moveHighlight(2);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            screen.moveHighlight(-1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            screen.moveHighlight(1);
            break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            screen.moveApp(-1);
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            screen.moveApp(1);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            if (const auto field = screen.choose()) {
                chooseResearch(app, *field);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// The galaxy map: the home star's name first, then the Turn button (also
// Enter, or the gamepad's Start); Esc or B goes back to the main menu.
SDL_AppResult mapEvent(App& app, SDL_Event* event) {
    if (app.researchDone) {
        return researchDoneEvent(app, event);
    }
    if (app.research) {
        return researchEvent(app, event);
    }
    if (app.colony) {
        return colonyEvent(app, event);
    }
    if (app.homeName) {
        auto& box = *app.homeName;
        switch (event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                box.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
            }
            return SDL_APP_CONTINUE;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT &&
                box.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                acceptHomeName(app);
            }
            return SDL_APP_CONTINUE;
        case SDL_EVENT_TEXT_INPUT:
            for (const char* c = event->text.text; *c != '\0'; ++c) {
                if (static_cast<unsigned char>(*c) < 0x80) { // the font has no others
                    box.type(*c);
                }
            }
            return SDL_APP_CONTINUE;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                acceptHomeName(app);
            } else if (event->key.key == SDLK_BACKSPACE) {
                box.backspace();
            }
            return SDL_APP_CONTINUE;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                acceptHomeName(app);
            }
            return SDL_APP_CONTINUE;
        default:
            return SDL_APP_CONTINUE;
        }
    }
    const bool turn =
        (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT &&
         hegemon::GalaxyMap::kTurnButton.contains(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) ||
        (event->type == SDL_EVENT_KEY_DOWN && (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER)) ||
        (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event->gbutton.button == SDL_GAMEPAD_BUTTON_START);
    if (turn) {
        endTurn(app);
        return SDL_APP_CONTINUE;
    }
    const bool back = (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) ||
                      (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST);
    if (back) {
        backToMainMenu(app);
    }
    return SDL_APP_CONTINUE;
}

// Hegemon's gamepad on the banner colours: the pointer jumps from banner to
// banner.
void stepBanner(App& app, int step) {
    constexpr int n = static_cast<int>(hegemon::BannerSelectScreen::kBanners);
    const auto current = app.banners->bannerAt(app.pointer->x(), app.pointer->y());
    int at = current ? static_cast<int>(*current) + step : (step > 0 ? 0 : n - 1);
    at = (at % n + n) % n;
    const auto rect = app.banners->bannerRect(static_cast<std::size_t>(at));
    app.pointer->move(rect.centerX(), rect.centerY());
}

SDL_AppResult bannerEvent(App& app, SDL_Event* event) {
    auto& screen = *app.banners;
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            screen.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (const auto colour = screen.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                chooseBanner(app, *colour);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) {
            leaveBanners(app);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            if (const auto colour = screen.click(app.pointer->x(), app.pointer->y())) {
                chooseBanner(app, *colour);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            stepBanner(app, -1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            stepBanner(app, 1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            stepBanner(app, 4);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            if (const auto colour = screen.click(app.pointer->x(), app.pointer->y())) {
                chooseBanner(app, *colour);
            }
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
            leaveBanners(app);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult rulerNameEvent(App& app, SDL_Event* event) {
    auto& dialog = *app.rulerName;
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            dialog.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT &&
            dialog.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
            acceptRulerName(app);
        }
        break;
    case SDL_EVENT_TEXT_INPUT:
        for (const char* c = event->text.text; *c != '\0'; ++c) {
            if (static_cast<unsigned char>(*c) < 0x80) { // the font has no others
                dialog.type(*c);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) {
            leaveRulerName(app);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            acceptRulerName(app);
        } else if (event->key.key == SDLK_BACKSPACE) {
            dialog.backspace();
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            acceptRulerName(app);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
            leaveRulerName(app);
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// Back from the race selection to the New Game screen.
void leaveRaceSelect(App& app) {
    app.raceSelect.reset();
    openNewGame(app);
}

SDL_AppResult raceSelectEvent(App& app, SDL_Event* event) {
    auto& screen = *app.raceSelect;
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        screen.pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            screen.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (const auto race = screen.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                chooseRace(app, *race);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) {
            leaveRaceSelect(app);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            if (const auto race = screen.click(app.pointer->x(), app.pointer->y())) {
                chooseRace(app, *race);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        app.pointer->hide();
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            screen.moveHighlight(-1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            screen.moveHighlight(1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            screen.moveHighlight(-7);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            screen.moveHighlight(7);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            chooseRace(app, screen.highlighted());
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
            leaveRaceSelect(app);
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
            const float v = static_cast<float>(event->gaxis.value) / 32767.0f;
            const int direction = v > kStickThreshold ? 1 : v < -kStickThreshold ? -1 : 0;
            if (direction != 0 && direction != app.stickDirection) {
                app.pointer->hide();
                screen.moveHighlight(direction);
            }
            app.stickDirection = direction;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult menuEvent(App& app, SDL_Event* event) {
    auto& menu = *app.menu;
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        menu.pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (menu.opening()) {
            menu.skipOpening(); // any button skips the opening animation
        } else if (event->button.button == SDL_BUTTON_LEFT) {
            menu.press(static_cast<int>(event->button.x), static_cast<int>(event->button.y));
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (const auto choice = menu.release(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                return choose(app, *choice);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (menu.opening()) {
            menu.skipOpening(); // any key skips the opening animation
        } else if (event->key.key == SDLK_UP || event->key.key == SDLK_DOWN) {
            app.pointer->hide();
            menu.moveHighlight(event->key.key == SDLK_UP ? -1 : 1);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            if (const auto choice = menu.activate()) {
                return choose(app, *choice);
            }
        } else if (event->key.key >= SDLK_A && event->key.key <= SDLK_Z) {
            if (const auto choice = menu.hotkey(static_cast<char>('a' + (event->key.key - SDLK_A)))) {
                return choose(app, *choice);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        app.pointer->hide();
        if (menu.opening()) {
            menu.skipOpening();
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
            menu.moveHighlight(-1);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
            menu.moveHighlight(1);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            if (const auto choice = menu.activate()) {
                return choose(app, *choice);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
            const float v = static_cast<float>(event->gaxis.value) / 32767.0f;
            const int direction = v > kStickThreshold ? 1 : v < -kStickThreshold ? -1 : 0;
            if (direction != 0 && direction != app.stickDirection) {
                app.pointer->hide();
                menu.moveHighlight(direction);
            }
            app.stickDirection = direction;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

} // namespace

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* app = static_cast<App*>(appstate);
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_GAMEPAD_ADDED:
        SDL_OpenGamepad(event->gdevice.which);
        return SDL_APP_CONTINUE;
    default:
        break;
    }
    if (app->menu == nullptr) {
        if (event->type == SDL_EVENT_KEY_DOWN) {
            return event->key.key == SDLK_ESCAPE ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
        }
        if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            return event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
        }
        return SDL_APP_CONTINUE;
    }
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        screenPoint(*app, event);
        app->pointer->move(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        screenPoint(*app, event);
        break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        app->pointer->hide();
        return SDL_APP_CONTINUE;
    default:
        break;
    }
    if (app->map) {
        return mapEvent(*app, event);
    }
    if (app->generating) {
        return generatingEvent(*app, event);
    }
    if (app->banners) {
        return bannerEvent(*app, event);
    }
    if (app->rulerName) {
        return rulerNameEvent(*app, event);
    }
    if (app->raceSelect) {
        return raceSelectEvent(*app, event);
    }
    return app->newGame ? newGameEvent(*app, event) : menuEvent(*app, event);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* app = static_cast<App*>(appstate);
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    const Uint64 now = SDL_GetTicks();
    const Uint64 elapsed = app->lastTicks == 0 ? 0 : now - app->lastTicks;
    app->lastTicks = now;
    if (app->menu) {
        if (app->buildList) {
            app->buildList->draw(app->canvas, app->palette);
        } else if (app->colony) {
            app->colony->tick(elapsed);
            app->colony->draw(app->canvas, app->palette);
            if (app->message) {
                app->message->draw(app->canvas, app->palette);
            }
        } else if (app->researchDone) {
            app->researchDone->tick(elapsed);
            app->researchDone->draw(app->canvas, app->palette);
        } else if (app->research) {
            app->research->tick(elapsed);
            app->research->draw(app->canvas, app->palette);
        } else if (app->map) {
            app->map->draw(app->canvas, app->palette);
            if (app->homeName) {
                app->homeName->tick(elapsed);
                app->homeName->draw(app->canvas);
            }
        } else if (app->generating) {
            if (app->shownGenerating) {
                makeGalaxy(*app);
            } else {
                app->generating->draw(app->canvas, app->palette);
                app->shownGenerating = true;
            }
        } else if (app->banners) {
            app->banners->draw(app->canvas, app->palette);
        } else if (app->rulerName) {
            app->rulerName->tick(elapsed);
            app->rulerName->draw(app->canvas, app->palette);
        } else if (app->raceSelect) {
            app->raceSelect->draw(app->canvas, app->palette);
        } else if (app->newGame) {
            app->newGame->draw(app->canvas, app->palette);
        } else {
            app->menu->tick(elapsed);
            app->menu->draw(app->canvas, app->palette);
        }
        app->pointer->draw(app->canvas);
        hegemon::toRgba(app->canvas, app->palette, app->rgba);
        SDL_UpdateTexture(app->frame, nullptr, app->rgba.data(), kScreenW * 4);
        SDL_RenderTexture(app->renderer, app->frame, nullptr, nullptr);
    } else {
        SDL_SetRenderDrawColor(app->renderer, 220, 220, 220, 255);
        float y = 16;
        for (const auto& line : app->lines) {
            SDL_RenderDebugText(app->renderer, 16, y, line.c_str());
            y += 12;
        }
    }
    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult) {
    auto* app = static_cast<App*>(appstate);
    if (app->frame != nullptr) {
        SDL_DestroyTexture(app->frame);
    }
    delete app;
}
