set(HGM_APP_SOURCES
    main.cpp
)

if(ANDROID)
    # SDLActivity loads the game as libmain.so.
    add_library(hgm_app SHARED ${HGM_APP_SOURCES})
    set_target_properties(hgm_app PROPERTIES OUTPUT_NAME main)
else()
    add_executable(hgm_app WIN32 MACOSX_BUNDLE ${HGM_APP_SOURCES})
    set_target_properties(hgm_app PROPERTIES
        OUTPUT_NAME Hegemon
        MACOSX_BUNDLE_GUI_IDENTIFIER io.github.niliodev.hegemon
        MACOSX_BUNDLE_BUNDLE_NAME Hegemon
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
endif()

target_link_libraries(hgm_app PRIVATE hegemon::core SDL3::SDL3)
hgm_set_warnings(hgm_app)
