// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_files.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

HGM_TEST("Game files: found without regard to case") {
    const fs::path folder = fs::temp_directory_path() / "hegemon_test_game_files";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::ofstream(folder / "mainmenu.lbx") << "x";
    HGM_CHECK(hegemon::findFileNoCase(folder, "MAINMENU.LBX").has_value());
    HGM_CHECK(!hegemon::findFileNoCase(folder, "ORION2.EXE").has_value());
    HGM_CHECK(!hegemon::isGameFolder(folder));
    std::ofstream(folder / "TechName.Lbx") << "x";
    HGM_CHECK(hegemon::isGameFolder(folder));
    fs::remove_all(folder);
    HGM_CHECK(!hegemon::isGameFolder(folder));
}
