// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Hegemon app. For now it opens a window at the original's 640×480,
// scaled sharply to the window, finds the player's game files and shows the
// picture of the main menu from them. Gamepads are opened as they are
// plugged in, so their buttons work from the first screen on.
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "hegemon/data/image.hpp"
#include "hegemon/data/lbx.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/version.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr int kScreenW = 640;
constexpr int kScreenH = 480;

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* picture = nullptr;
    std::vector<std::string> lines;
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

void checkGameFiles(App& app, const std::filesystem::path& folder) {
    std::size_t archives = 0;
    std::size_t entries = 0;
    std::size_t failed = 0;
    std::error_code ec;
    for (const auto& file : std::filesystem::directory_iterator(folder, ec)) {
        std::string ext = file.path().extension().string();
        for (auto& c : ext) {
            c = static_cast<char>(SDL_toupper(c));
        }
        if (ext != ".LBX") {
            continue;
        }
        try {
            const auto archive = hegemon::LbxArchive::load(file.path());
            ++archives;
            entries += archive.size();
        } catch (const hegemon::FormatError&) {
            ++failed; // the videos, until they get their own reader
        }
    }
    app.lines.push_back("Game files: " + folder.string());
    app.lines.push_back(std::to_string(archives) + " archives with " + std::to_string(entries) + " entries read.");
    app.lines.push_back(std::to_string(failed) + " files are videos or could not be read.");
}

// The picture behind the original's main menu, in its own colours.
void showMainMenu(App& app, const std::filesystem::path& folder) {
    const auto file = hegemon::findFileNoCase(folder, "MAINMENU.LBX");
    if (!file) {
        return;
    }
    try {
        const auto archive = hegemon::LbxArchive::load(*file);
        const auto image = hegemon::Image::parse(archive.entry(1));
        hegemon::Palette palette;
        image.applyPalette(palette);
        hegemon::Canvas canvas(kScreenW, kScreenH);
        image.draw(0, canvas);
        std::vector<std::uint8_t> rgba(static_cast<std::size_t>(kScreenW) * kScreenH * 4);
        hegemon::toRgba(canvas, palette, rgba);
        app.picture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, kScreenW,
                                        kScreenH);
        if (app.picture != nullptr) {
            SDL_SetTextureScaleMode(app.picture, SDL_SCALEMODE_NEAREST);
            SDL_UpdateTexture(app.picture, nullptr, rgba.data(), kScreenW * 4);
        }
    } catch (const hegemon::FormatError& e) {
        app.lines.push_back(std::string("The main menu picture could not be read: ") + e.what());
    }
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

    app->lines.push_back("Hegemon " + std::string(hegemon::kVersion));
    std::optional<std::filesystem::path> folder;
    if (argc > 1) {
        folder = std::filesystem::path(argv[1]);
    } else {
        folder = findGameFolder();
    }
    if (folder && hegemon::isGameFolder(*folder)) {
        checkGameFiles(*app, *folder);
        showMainMenu(*app, *folder);
    } else {
        app->lines.push_back("Put your own copy of the original game in a folder");
        app->lines.push_back("named gamedata next to Hegemon and start it again.");
    }
    app->lines.push_back("");
    app->lines.push_back("Esc or the gamepad's B button closes this window.");
    for (const auto& line : app->lines) {
        SDL_Log("%s", line.c_str());
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    (void)appstate;
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_GAMEPAD_ADDED:
        SDL_OpenGamepad(event->gdevice.which);
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
            return SDL_APP_SUCCESS;
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
    if (app->picture != nullptr) {
        SDL_RenderTexture(app->renderer, app->picture, nullptr, nullptr);
        SDL_RenderPresent(app->renderer);
        return SDL_APP_CONTINUE;
    }
    SDL_SetRenderDrawColor(app->renderer, 220, 220, 220, 255);
    float y = 16;
    for (const auto& line : app->lines) {
        SDL_RenderDebugText(app->renderer, 16, y, line.c_str());
        y += 12;
    }
    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult) {
    auto* app = static_cast<App*>(appstate);
    if (app->picture != nullptr) {
        SDL_DestroyTexture(app->picture);
    }
    delete app;
}
