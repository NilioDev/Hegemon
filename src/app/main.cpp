// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Hegemon app: opens a window, turns mouse, keyboard and gamepad input
// into commands for the current screen, and shows the 640×480 picture the
// core draws, scaled sharply to the window. For now the screens are the
// main menu and the New Game screen.
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "hegemon/data/image.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/main_menu.hpp"
#include "hegemon/ui/new_game.hpp"
#include "hegemon/ui/pointer.hpp"
#include "hegemon/version.hpp"

#include <filesystem>
#include <memory>
#include <optional>
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

// What a choice in the main menu does.
SDL_AppResult choose(App& app, hegemon::MainMenu::Choice choice) {
    using Choice = hegemon::MainMenu::Choice;
    if (choice == Choice::Quit) {
        return SDL_APP_SUCCESS;
    }
    if (choice == Choice::NewGame) {
        try {
            app.newGame = std::make_unique<hegemon::NewGameScreen>(*app.data, app.settings);
            app.pointer->useEntry(hegemon::NewGameScreen::kPointerEntry);
        } catch (const hegemon::FormatError& e) {
            SDL_Log("The New Game screen could not be read: %s", e.what());
        }
        return SDL_APP_CONTINUE;
    }
    SDL_Log("This part of the game is not there yet.");
    return SDL_APP_CONTINUE;
}

// Leaves the New Game screen for the main menu.
void finishNewGame(App& app, hegemon::NewGameScreen::Result result) {
    if (result == hegemon::NewGameScreen::Result::Accept) {
        app.settings = app.newGame->settings();
        SDL_Log("Choosing a race is not there yet.");
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
        if (app->newGame) {
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
