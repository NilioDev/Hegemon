add_executable(hgm_tests
    test_main.cpp
    test_game_files.cpp
    test_lbx.cpp
    test_image.cpp
    test_main_menu.cpp
    test_new_game.cpp
    test_races.cpp
    test_race_select.cpp
    test_ruler_name.cpp
    test_galaxy_map.cpp
    test_banner_select.cpp
    test_parameters.cpp
    test_font.cpp
    test_credits.cpp
    test_config.cpp
    test_galaxy.cpp
    test_empire.cpp
    test_research.cpp
    test_research_done.cpp
    test_research_select.cpp
    test_ship_design.cpp
    test_colony_screen.cpp
    test_build_list.cpp
    test_buy.cpp
    test_fleet.cpp
    test_fleet_window.cpp
    test_system_window.cpp
)
target_link_libraries(hgm_tests PRIVATE hegemon::core)
target_compile_definitions(hgm_tests PRIVATE HEGEMON_GAMEDATA_DIR="${PROJECT_SOURCE_DIR}/gamedata")
hgm_set_warnings(hgm_tests)

add_test(NAME hgm_tests COMMAND hgm_tests)
