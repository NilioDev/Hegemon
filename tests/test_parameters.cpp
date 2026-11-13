// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/json.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

template <typename F> bool throws(F&& f) {
    try {
        f();
    } catch (const hegemon::FormatError&) {
        return true;
    }
    return false;
}

// A list like the fan patch's, cut down.
constexpr const char* kList = R"({"patch_version": "1.51", "parameters": [
 {"name": "star_special_chance", "type": "integer", "ctype": "uint8_t", "min": 0, "max": 100, "default": 64},
 {"name": "steal_mode", "type": "enum", "default": "except_current_field",
  "values": ["except_current_field", "except_current_tech", "anything"]},
 {"name": "stars_in_galaxy", "type": "table", "rows": [], "default": [20, 36, 54, 71], "columns": [
  {"name": "small", "type": "integer", "min": 3, "max": 71}, {"name": "medium", "type": "integer", "min": 3, "max": 71},
  {"name": "large", "type": "integer", "min": 3, "max": 71}, {"name": "huge", "type": "integer", "min": 3, "max": 71}]},
 {"name": "star_class_chance", "type": "table", "rows": [["blue", "white"]], "default": [[20, 10, 5], [25, 15, 5]],
  "columns": [{"name": "young", "type": "integer"}, {"name": "average", "type": "integer"}, {"name": "old", "type": "integer"}]}
]})";

hegemon::FanPatchConfig configWith(const std::string& text) {
    const fs::path folder = fs::temp_directory_path() / "hegemon_test_parameters";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::ofstream(folder / "ORION2.CFG") << text;
    auto c = hegemon::FanPatchConfig::load(folder);
    fs::remove_all(folder);
    return c;
}

} // namespace

HGM_TEST("JSON: values, escapes and damage") {
    const auto v = hegemon::JsonValue::parse(R"( {"a": [1, -2.5e1, true, null, "xé\n"], "b": {}} )");
    const auto& a = v.find("a")->array();
    HGM_CHECK(a.size() == 5);
    HGM_CHECK(a[0].integer() == 1);
    HGM_CHECK(a[1].number() == -25.0);
    HGM_CHECK(a[2].boolean());
    HGM_CHECK(a[3].isNull());
    HGM_CHECK(a[4].string() == "x\xc3\xa9\n");
    HGM_CHECK(v.find("b")->object().empty());
    HGM_CHECK(v.find("c") == nullptr);
    HGM_CHECK(throws([] { (void)hegemon::JsonValue::parse(R"({"a": 1,})"); }));
    HGM_CHECK(throws([] { (void)hegemon::JsonValue::parse(R"("open)"); }));
    HGM_CHECK(throws([] { (void)hegemon::JsonValue::parse("[1] 2"); }));
    HGM_CHECK(throws([] { (void)hegemon::JsonValue::parse(std::string(100, '[')); }));
    HGM_CHECK(throws([] { (void)hegemon::JsonValue::parse("1.5").integer(); }));
}

HGM_TEST("Parameters: defaults from the list, changed by the settings") {
    const auto d = hegemon::Parameters::defaults(kList);
    HGM_CHECK(d.version() == "1.51");
    HGM_CHECK(d.integer("star_special_chance") == 64);
    HGM_CHECK(d.integer("steal_mode") == 0);
    HGM_CHECK(d.integer("stars_in_galaxy", {}, "huge") == 71);
    HGM_CHECK(d.integer("star_class_chance", {"white"}, "old") == 5);
    HGM_CHECK(d.integer("star_class_chance", 0, 1) == 10);

    const auto p = hegemon::Parameters::load(kList, configWith("star_special_chance = 50;\n"
                                                               "steal_mode = anything;\n"
                                                               "stars_in_galaxy medium = 40;\n"
                                                               "star_class_chance white = 1 2 3;\n"
                                                               "star_class_chance blue young = 7;\n"
                                                               "something_else = 5;\n"));
    HGM_CHECK(p.integer("star_special_chance") == 50);
    HGM_CHECK(p.integer("steal_mode") == 2);
    HGM_CHECK(p.integer("stars_in_galaxy", {}, "medium") == 40);
    HGM_CHECK(p.integer("star_class_chance", {"white"}, "average") == 2);
    HGM_CHECK(p.integer("star_class_chance", {"blue"}, "young") == 7);
    HGM_CHECK(p.integer("star_class_chance", {"blue"}, "old") == 5);

    for (const char* bad : {"star_special_chance = 101;", "steal_mode = never;", "stars_in_galaxy tiny = 5;",
                            "star_class_chance white = 1 2;", "star_class_chance = 1 2 3;",
                            "stars_in_galaxy small = 2;"}) {
        HGM_CHECK(throws([&] { (void)hegemon::Parameters::load(kList, configWith(bad)); }));
    }
}

HGM_TEST("Parameters: the player's fan patch 1.51") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game) || !fs::exists(game / "150" / "docs" / "config.json")) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    std::ifstream in(game / "150" / "docs" / "config.json", std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    const auto config = hegemon::FanPatchConfig::load(game);
    const auto p = hegemon::Parameters::load(text.str(), config);
    HGM_CHECK(p.version() == "1.51");
    HGM_CHECK(p.integer("stars_in_galaxy", {}, "small") == 20);
    HGM_CHECK(p.integer("star_class_chance", {"black_hole"}, "old") == 6);
    HGM_CHECK(p.integer("map_generation", {"huge"}, "width") == 1518);
    HGM_CHECK(p.integer("planet_special_chance", {}, "natives") == 9);

    hegemon::GameData data(game);
    HGM_CHECK(data.parameters().integer("star_special_chance") == p.integer("star_special_chance"));
    HGM_CHECK(hegemon::findPathNoCase(game, "150/DOCS/Config.JSON").has_value());
    HGM_CHECK(!hegemon::findPathNoCase(game, "150/nothing/config.json"));
}
