# EasyMorse desktop application (Qt Widgets)

# =======================================================================
# Executable

qt_add_executable(easymorse WIN32 MACOSX_BUNDLE
    main.cpp

    gui/mainwindow.cpp gui/mainwindow.h gui/mainwindow.ui
    gui/options.cpp gui/options.h gui/options.ui

    "${PROJECT_SOURCE_DIR}/resources.qrc"
)

target_include_directories(easymorse PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/generated"
)

target_link_libraries(easymorse PRIVATE
    easymorse_core
    Qt6::Widgets
)

set_target_properties(easymorse PROPERTIES
    OUTPUT_NAME easymorse
    # Keep the executable at the top of the build tree
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    MACOSX_BUNDLE_BUNDLE_NAME "EasyMorse"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.artemia76.easymorse"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE_ICON_FILE easymorse.icns
)

if(WIN32)
    target_sources(easymorse PRIVATE "${PROJECT_SOURCE_DIR}/EasyMorse.rc")
elseif(APPLE)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/gfx/easymorse.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(easymorse PRIVATE "${PROJECT_SOURCE_DIR}/gfx/easymorse.icns")
    set_target_properties(easymorse PROPERTIES MACOSX_DEPLOYMENT_TARGET 12.0)
endif()

# Translations are compiled and embedded under :/i18n
qt_add_translations(easymorse
    SOURCE_TARGETS easymorse easymorse_core easymorse-qml
    TS_FILES "${PROJECT_SOURCE_DIR}/intl/easymorse_fr_FR.ts"
    RESOURCE_PREFIX /i18n
)

# =======================================================================
# Installation

include(GNUInstallDirs)

install(TARGETS easymorse
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/easymorse.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES "${PROJECT_SOURCE_DIR}/gfx/Morse_256x256.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME easymorse.png)
endif()

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.GPLv3" "${PROJECT_SOURCE_DIR}/README.md" DESTINATION ${CMAKE_INSTALL_DOCDIR})

if(EASYMORSE_DEPLOY_QT)
    qt_generate_deploy_app_script(
        TARGET easymorse
        OUTPUT_SCRIPT EASYMORSE_DEPLOY_SCRIPT
    )
    install(SCRIPT "${EASYMORSE_DEPLOY_SCRIPT}")
    if(WIN32)
        # Ship the MSVC runtime next to the executable
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
        include(InstallRequiredSystemLibraries)
    endif()
endif()
