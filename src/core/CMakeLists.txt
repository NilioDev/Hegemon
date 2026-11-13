configure_file(version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/hegemon/version.hpp" @ONLY)

add_library(hgm_core STATIC
    src/game_files.cpp
    src/game_data.cpp
    src/ui/main_menu.cpp
    src/ui/new_game.cpp
    src/ui/pointer.cpp
    src/ui/race_select.cpp
    src/ui/ruler_name.cpp
    src/ui/banner_select.cpp
    src/rules/races.cpp
    src/data/lbx.cpp
    src/data/image.cpp
    src/data/font.cpp
    src/data/text_table.cpp
    src/data/pointers.cpp
    src/ui/credits.cpp
    src/config/fan_patch_config.cpp
)
add_library(hegemon::core ALIAS hgm_core)

target_include_directories(hgm_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_features(hgm_core PUBLIC cxx_std_20)
set_target_properties(hgm_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
hgm_set_warnings(hgm_core)
