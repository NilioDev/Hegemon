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
    test_font.cpp
    test_credits.cpp
    test_config.cpp
)
target_link_libraries(hgm_tests PRIVATE hegemon::core)
target_compile_definitions(hgm_tests PRIVATE HEGEMON_GAMEDATA_DIR="${PROJECT_SOURCE_DIR}/gamedata")
hgm_set_warnings(hgm_tests)

add_test(NAME hgm_tests COMMAND hgm_tests)
