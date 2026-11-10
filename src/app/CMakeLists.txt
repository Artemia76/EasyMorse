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
        qml/SafeAreaProbe.qml
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

if(ANDROID)
    # Manifest, icons and theme; Qt fills in the rest of the Gradle project
    math(EXPR EASYMORSE_VERSION_CODE
        "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
    set_target_properties(easymorse PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android"
        QT_ANDROID_PACKAGE_NAME "io.github.artemia76.easymorse"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 35
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE ${EASYMORSE_VERSION_CODE}
    )
    # Android gives applications no OpenSSL: without it no https:// or wss://
    # server. Built from the official sources by packaging/android/make-openssl.sh
    set(EASYMORSE_ANDROID_OPENSSL_DIR "" CACHE PATH
        "OpenSSL built by packaging/android/make-openssl.sh, needed for https:// and wss://")
    if(EASYMORSE_ANDROID_OPENSSL_DIR)
        set(_openssl "${EASYMORSE_ANDROID_OPENSSL_DIR}/${CMAKE_ANDROID_ARCH_ABI}")
        if(NOT EXISTS "${_openssl}/libssl_3.so" OR NOT EXISTS "${_openssl}/libcrypto_3.so")
            message(FATAL_ERROR "No OpenSSL for ${CMAKE_ANDROID_ARCH_ABI} in ${EASYMORSE_ANDROID_OPENSSL_DIR}: "
                                "run packaging/android/make-openssl.sh")
        endif()
        # Loaded by Qt at run time, not linked
        set_property(TARGET easymorse APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
            "${_openssl}/libcrypto_3.so" "${_openssl}/libssl_3.so")
    else()
        message(WARNING "EASYMORSE_ANDROID_OPENSSL_DIR is not set: the application will not "
                        "reach https:// servers (see packaging/android/make-openssl.sh)")
    endif()
elseif(IOS)
    set_target_properties(easymorse PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.github.artemia76.easymorse"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        # Direct connections to the stations of the local network (merged by Xcode with Qt's Info.plist)
        XCODE_ATTRIBUTE_GENERATE_INFOPLIST_FILE YES
        XCODE_ATTRIBUTE_INFOPLIST_KEY_NSLocalNetworkUsageDescription
            "EasyMorse connects directly to the other stations of your local network for live Morse sessions."
    )
elseif(WIN32)
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
    # Strings removed from the sources leave the translation file
    LUPDATE_OPTIONS -no-obsolete
)

# =======================================================================
# Installation

include(GNUInstallDirs)

if(EASYMORSE_MOBILE)
    # The APK or the iOS application is built by Qt, nothing to install
    return()
endif()

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

if(APPLE)
    # Inside the bundle: a disk image shows the application and nothing else
    set(_doc_destination "easymorse.app/Contents/Resources")
else()
    set(_doc_destination "${CMAKE_INSTALL_DOCDIR}")
endif()
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.GPLv3" "${PROJECT_SOURCE_DIR}/README.md" DESTINATION "${_doc_destination}")

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
    if(APPLE)
        # Deploying modifies Qt's signed libraries, and Apple Silicon Macs
        # refuse code whose signature is broken: sign the whole bundle again
        install(CODE [[
            execute_process(
                COMMAND codesign --force --deep --sign - "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/easymorse.app"
                COMMAND_ERROR_IS_FATAL ANY
            )
        ]])
    endif()
endif()
