// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Hegemon app: opens a window, turns mouse, keyboard and gamepad input
// into commands for the current screen, and shows the 640×480 picture the
// core draws, scaled sharply to the window. For now the only screen is the
// main menu.
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "hegemon/data/image.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/main_menu.hpp"
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

// What a choice in the main menu does. Only Quit leads anywhere yet.
SDL_AppResult choose(hegemon::MainMenu::Choice choice) {
    if (choice == hegemon::MainMenu::Choice::Quit) {
        return SDL_APP_SUCCESS;
    }
    SDL_Log("This part of the game is not there yet.");
    return SDL_APP_CONTINUE;
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

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* app = static_cast<App*>(appstate);
    auto* menu = app->menu.get();
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_GAMEPAD_ADDED:
        SDL_OpenGamepad(event->gdevice.which);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (menu != nullptr) {
            screenPoint(*app, event);
            menu->pointerMoved(static_cast<int>(event->motion.x), static_cast<int>(event->motion.y));
        }
        break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        if (menu != nullptr) {
            menu->hidePointer();
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (menu != nullptr && menu->opening()) {
            menu->skipOpening(); // any button skips the opening animation
        } else if (menu != nullptr && event->button.button == SDL_BUTTON_LEFT) {
            screenPoint(*app, event);
            if (const auto choice = menu->click(static_cast<int>(event->button.x), static_cast<int>(event->button.y))) {
                return choose(*choice);
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (menu == nullptr) {
            return event->key.key == SDLK_ESCAPE ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
        }
        if (menu->opening()) {
            menu->skipOpening(); // any key skips the opening animation
        } else if (event->key.key == SDLK_UP || event->key.key == SDLK_DOWN) {
            menu->hidePointer();
            menu->moveHighlight(event->key.key == SDLK_UP ? -1 : 1);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            if (const auto choice = menu->activate()) {
                return choose(*choice);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (menu == nullptr) {
            return event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
        }
        menu->hidePointer();
        if (menu->opening()) {
            menu->skipOpening();
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
            menu->moveHighlight(-1);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
            menu->moveHighlight(1);
        } else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            if (const auto choice = menu->activate()) {
                return choose(*choice);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (menu != nullptr && event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
            const float v = static_cast<float>(event->gaxis.value) / 32767.0f;
            const int direction = v > kStickThreshold ? 1 : v < -kStickThreshold ? -1 : 0;
            if (direction != 0 && direction != app->stickDirection) {
                menu->hidePointer();
                menu->moveHighlight(direction);
            }
            app->stickDirection = direction;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* app = static_cast<App*>(appstate);
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    const Uint64 now = SDL_GetTicks();
    const Uint64 elapsed = app->lastTicks == 0 ? 0 : now - app->lastTicks;
    app->lastTicks = now;
    if (app->menu) {
        app->menu->tick(elapsed);
        app->menu->draw(app->canvas, app->palette);
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
