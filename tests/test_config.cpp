// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/fan_patch_config.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using hegemon::ConfigError;
using hegemon::ConfigValue;
using hegemon::FanPatchConfig;

namespace {

fs::path freshFolder() {
    const fs::path folder = fs::temp_directory_path() / "hegemon_test_config";
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

template <typename F> bool throwsConfigError(F&& f) {
    try {
        f();
    } catch (const ConfigError&) {
        return true;
    }
    return false;
}

} // namespace

HGM_TEST("Settings: statements, values, texts and comments") {
    const auto st = hegemon::parseConfig("# a comment\n"
                                         "  format version = \"Version $VERSION$\" 1 r 541 433; # label\n"
                                         "font_color_group g0 normal base = 0xBB;\r\n"
                                         "x=-2;include ?\\150\\ENABLE.CFG;\n"
                                         "enable USER;",
                                         "TEST.CFG");
    HGM_CHECK(st.size() == 5);
    HGM_CHECK(st[0].assignment);
    HGM_CHECK(st[0].line == 2);
    HGM_CHECK(st[0].keys.size() == 2 && st[0].keys[0] == "format" && st[0].keys[1] == "version");
    HGM_CHECK(st[0].values.size() == 5);
    HGM_CHECK(st[0].values[0].kind == ConfigValue::Kind::Text);
    HGM_CHECK(st[0].values[0].text == "Version $VERSION$");
    HGM_CHECK(st[0].values[1].kind == ConfigValue::Kind::Number && st[0].values[1].number == 1);
    HGM_CHECK(st[0].values[2].kind == ConfigValue::Kind::Word && st[0].values[2].text == "r");
    HGM_CHECK(st[1].keys.size() == 4 && st[1].values[0].number == 0xBB);
    HGM_CHECK(st[2].keys[0] == "x" && st[2].values[0].number == -2);
    HGM_CHECK(!st[3].assignment && st[3].keys[0] == "include" && st[3].values[0].text == "?\\150\\ENABLE.CFG");
    HGM_CHECK(!st[4].assignment && st[4].keys[0] == "enable" && st[4].values[0].text == "USER");

    HGM_CHECK(throwsConfigError([] { (void)hegemon::parseConfig("a = \"open;", "T"); }));
    HGM_CHECK(throwsConfigError([] { (void)hegemon::parseConfig("a = 1", "T"); }));
    HGM_CHECK(throwsConfigError([] { (void)hegemon::parseConfig("= 1;", "T"); }));
    HGM_CHECK(throwsConfigError([] { (void)hegemon::parseConfig("a = 1 = 2;", "T"); }));
}

HGM_TEST("Settings: includes, enabled mods by their order, the last value wins") {
    const fs::path folder = freshFolder();
    write(folder / "ORION2.CFG", "scan_mods = \\150\\mods;\n"
                                 "include ?\\150\\ENABLE.CFG;\n"
                                 "include ?\\150\\MISSING.CFG;\n"
                                 "enable USER;\n"
                                 "help.lbx = ?\\150\\lbx\\$LANG$\\HELP.LBX;\n"
                                 "warn_blockers = 1;\n");
    write(folder / "150" / "enable.cfg", "enable core_b; enable core_a;\n");
    write(folder / "150" / "USER.CFG", "warn_blockers = 3;\n");
    write(folder / "150" / "lbx" / "en" / "help.lbx", "x");
    write(folder / "150" / "mods" / "a" / "A.CFG", "mod_id = core_a; mod_class = Core; mod_order = 0;\n"
                                                   "mod_name = \"Mod A\";\n"
                                                   "include cfg\\TWEAKS.CFG;\n"
                                                   "enable MAP_A;\n"
                                                   "combat.lbx = ..\\..\\lbx\\COMBAT.LBX;\n");
    write(folder / "150" / "mods" / "a" / "cfg" / "TWEAKS.CFG", "warn_blockers = 2;\n");
    write(folder / "150" / "mods" / "b" / "B.CFG", "mod_id = core_b; mod_class = core; mod_order = 5;\n"
                                                   "warn_blockers = 9;\n");
    write(folder / "150" / "mods" / "maps" / "MAP.CFG", "mod_id = MAP_A; mod_class = map;\nmapgen x = 1;\n");
    write(folder / "150" / "mods" / "user" / "USER.CFG", "mod_id = USER; mod_class = _user; mod_order = 2000;\n"
                                                         "include \\150\\USER.CFG;\n");
    write(folder / "150" / "lbx" / "COMBAT.LBX", "x");

    const auto config = FanPatchConfig::load(folder);
    // Core A comes first by its order; B is of the same class and left out.
    HGM_CHECK(config.mods().size() == 3);
    HGM_CHECK(config.mods()[0].id == "core_a" && config.mods()[0].name == "Mod A");
    HGM_CHECK(config.mods()[1].id == "MAP_A" && config.mods()[1].order == 200);
    HGM_CHECK(config.mods()[2].id == "USER");
    const auto* warn = config.last({"warn_blockers"});
    HGM_CHECK(warn != nullptr && warn->values[0].number == 3);
    HGM_CHECK(config.startingWith({"warn_blockers"}).size() == 3);
    HGM_CHECK(config.last({"mapgen", "x"}) != nullptr);

    const auto combat = config.lbxFile("COMBAT.LBX");
    HGM_CHECK(combat && fs::equivalent(*combat, folder / "150" / "lbx" / "COMBAT.LBX"));
    const auto help = config.lbxFile("HELP.LBX");
    HGM_CHECK(help && fs::equivalent(*help, folder / "150" / "lbx" / "en" / "help.lbx"));
    HGM_CHECK(!config.lbxFile("MAINMENU.LBX"));

    // The game data opens the replacement instead of the folder's own file.
    write(folder / "COMBAT.LBX", "y");
    write(folder / "150" / "lbx" / "COMBAT.LBX", std::string(16, '\0'));
    hegemon::GameData data(folder);
    HGM_CHECK(data.config() != nullptr);
    bool threw = false;
    try {
        (void)data.archive("COMBAT.LBX");
    } catch (const hegemon::FormatError&) {
        threw = true; // the replacement is no LBX archive; the point is which file it read
    }
    HGM_CHECK(threw);
    fs::remove_all(folder);
}

HGM_TEST("Settings: missing files, paths out of the game folder, include loops") {
    fs::path folder = freshFolder();
    HGM_CHECK(throwsConfigError([&] { (void)FanPatchConfig::load(folder); }));
    HGM_CHECK(hegemon::GameData(folder).config() == nullptr);

    write(folder / "ORION2.CFG", "include \\150\\MISSING.CFG;\n");
    HGM_CHECK(throwsConfigError([&] { (void)FanPatchConfig::load(folder); }));

    write(folder / "ORION2.CFG", "include ..\\outside.cfg;\n");
    HGM_CHECK(throwsConfigError([&] { (void)FanPatchConfig::load(folder); }));

    write(folder / "ORION2.CFG", "include ORION2.CFG;\n");
    HGM_CHECK(throwsConfigError([&] { (void)FanPatchConfig::load(folder); }));

    write(folder / "ORION2.CFG", "combat.lbx = \\150\\NOPE.LBX;\n");
    const auto config = FanPatchConfig::load(folder);
    HGM_CHECK(throwsConfigError([&] { (void)config.lbxFile("COMBAT.LBX"); }));
    fs::remove_all(folder);
}

HGM_TEST("Settings: the player's fan patch 1.51 loads") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::findFileNoCase(game, FanPatchConfig::kMainFile)) {
        HGM_SKIP("fan patch 1.51 not installed in gamedata/");
    }
    const auto config = FanPatchConfig::load(game);
    HGM_CHECK(!config.mods().empty());
    HGM_CHECK(config.last({"format", "version"}) != nullptr);
}
