# EasyMorse application: QML user interface for desktop and mobile

# QML files live in the qml/ subdirectory
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

# =======================================================================
# Executable

qt_add_executable(easymorse WIN32 MACOSX_BUNDLE
    main.cpp
    "${PROJECT_SOURCE_DIR}/resources.qrc"
)

# Colors and fonts shared by every page
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(easymorse
    URI EasyMorse
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ControlsPanel.qml
        qml/LabeledSlider.qml
        qml/KeyPad.qml
        qml/SendPage.qml
        qml/KeyPage.qml
        qml/NetworkPage.qml
        qml/ServerPanel.qml
        qml/DecoderPanel.qml
        qml/GlossaryPage.qml
        qml/LogPage.qml
        qml/OptionsPage.qml
        qml/Theme.qml
        qml/Card.qml
        qml/Chip.qml
        qml/NavButton.qml
        qml/Segmented.qml
        qml/PaddlePad.qml
    RESOURCES
        icons/send.svg
        icons/key.svg
        icons/network.svg
        icons/glossary.svg
        icons/log.svg
        icons/options.svg
        icons/sliders.svg
        icons/more.svg
        icons/arrow.svg
        icons/play.svg
        icons/stop.svg
    SOURCES
        qmltypes.h
        glossarymodel.cpp glossarymodel.h
        keyboardkeyer.cpp keyboardkeyer.h
        logmodel.cpp logmodel.h
        uisettings.cpp uisettings.h
)

target_include_directories(easymorse PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/generated"
)

target_link_libraries(easymorse PRIVATE
    easymorse_core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
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
    SOURCE_TARGETS easymorse easymorse_core
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
    # Also deploys the QML modules used by the interface
    qt_generate_deploy_qml_app_script(
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
