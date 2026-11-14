configure_file(version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/hegemon/version.hpp" @ONLY)

add_library(hgm_core STATIC
    src/game_files.cpp
    src/game_data.cpp
    src/ui/main_menu.cpp
    src/ui/new_game.cpp
    src/ui/pointer.cpp
    src/ui/race_select.cpp
    src/ui/ruler_name.cpp
    src/ui/name_box.cpp
    src/ui/galaxy_map.cpp
    src/ui/research_done.cpp
    src/ui/research_select.cpp
    src/ui/colony_screen.cpp
    src/ui/build_list.cpp
    src/ui/message_box.cpp
    src/ui/product_picture.cpp
    src/ui/banner_select.cpp
    src/ui/fleet_window.cpp
    src/ui/system_window.cpp
    src/rules/races.cpp
    src/data/lbx.cpp
    src/data/image.cpp
    src/data/font.cpp
    src/data/text_table.cpp
    src/data/pointers.cpp
    src/ui/credits.cpp
    src/config/fan_patch_config.cpp
    src/config/parameters.cpp
    src/rules/random.cpp
    src/rules/galaxy_generator.cpp
    src/rules/empire.cpp
    src/rules/tech.cpp
    src/rules/ship_design.cpp
    src/rules/production.cpp
    src/rules/fleet.cpp
    src/data/json.cpp
)
add_library(hegemon::core ALIAS hgm_core)

target_include_directories(hgm_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_features(hgm_core PUBLIC cxx_std_20)
set_target_properties(hgm_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
hgm_set_warnings(hgm_core)
