#****************************************************************************
#
# Copyright (C) 2020 Gianni Peschiutta (F4IKZ)
# Contact: https://github.com/Artemia76/EasyMorse
#
# EasyMorse is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# EasyMorse is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The license is as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL3
# included in the packaging of this software. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#****************************************************************************

cmake_minimum_required(VERSION 3.21)

project(EasyMorse
    VERSION 1.1.0
    DESCRIPTION "Learn Morse code by ear and by hand"
    HOMEPAGE_URL "https://github.com/Artemia76/EasyMorse"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =======================================================================
# Options

option(EASYMORSE_FETCH_PORTAUDIO "Download and build PortAudio instead of using the system one" OFF)
if(WIN32 OR APPLE)
    set(_deploy_qt_default ON)
else()
    # Linux distributions provide Qt themselves
    set(_deploy_qt_default OFF)
endif()
option(EASYMORSE_DEPLOY_QT "Bundle Qt libraries and plugins when installing" ${_deploy_qt_default})

# =======================================================================
# Dependencies

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Gui Widgets SerialPort LinguistTools)
qt_standard_project_setup(REQUIRES 6.7 I18N_SOURCE_LANGUAGE en I18N_TRANSLATED_LANGUAGES fr_FR)
set(CMAKE_AUTORCC ON)

include(cmake/PortAudio.cmake)
add_subdirectory(3rdparty/miniaudio)

# =======================================================================
# Version

set(EASYMORSE_GIT_REVISION "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE EASYMORSE_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

configure_file(tools/version.h.in "${CMAKE_BINARY_DIR}/generated/tools/version.h" @ONLY)

# =======================================================================
# Application

qt_add_executable(easymorse WIN32 MACOSX_BUNDLE
    main.cpp

    audio/Audio.cpp audio/Audio.h
    audio/DefaultParameters.h
    audio/IOscillatorFunction.h
    audio/Saw.cpp
    audio/Sine.cpp
    audio/Square.cpp
    audio/Triangle.cpp
    audio/envelope/Envelope.cpp audio/envelope/Envelope.h
    audio/envelope/IEnvelope.h
    audio/envelope/NoEnvelope.h
    audio/filter/BandpassFilter.cpp audio/filter/BandpassFilter.h
    audio/filter/FilterTypes.h
    audio/filter/HighpassFilter.cpp audio/filter/HighpassFilter.h
    audio/filter/IFilter.h
    audio/filter/LowpassFilter.cpp audio/filter/LowpassFilter.h
    audio/filter/NoFilter.h
    audio/modulation/ModulationValue.cpp audio/modulation/ModulationValue.h
    audio/oscillator/ILfo.h
    audio/oscillator/Lfo.cpp audio/oscillator/Lfo.h
    audio/oscillator/NoOscillator.h
    audio/voice/INoise.h
    audio/voice/IVoice.h
    audio/voice/Noise.cpp audio/voice/Noise.h
    audio/voice/Voice.cpp audio/voice/Voice.h
    audio/voicemanager/Param.cpp audio/voicemanager/Param.h
    audio/voicemanager/VoiceManager.cpp audio/voicemanager/VoiceManager.h

    gui/mainwindow.cpp gui/mainwindow.h gui/mainwindow.ui
    gui/options.cpp gui/options.h gui/options.ui

    morse/analyze.cpp morse/analyze.h
    morse/morse.cpp morse/morse.h

    tools/call_once.h
    tools/clogger.cpp tools/clogger.h
    tools/datetime.cpp tools/datetime.h
    tools/singleton.h
    "${CMAKE_BINARY_DIR}/generated/tools/version.h"

    resources.qrc
)

target_include_directories(easymorse PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(easymorse PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    EasyMorse::PortAudio
)

target_compile_definitions(easymorse PRIVATE
    QT_DISABLE_DEPRECATED_UP_TO=0x060700
)

if(MSVC)
    target_compile_options(easymorse PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(easymorse PRIVATE -Wall -Wextra)
endif()

set_target_properties(easymorse PROPERTIES
    OUTPUT_NAME easymorse
    MACOSX_BUNDLE_BUNDLE_NAME "EasyMorse"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.artemia76.easymorse"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE_ICON_FILE easymorse.icns
)

if(WIN32)
    target_sources(easymorse PRIVATE EasyMorse.rc)
elseif(APPLE)
    set_source_files_properties(gfx/easymorse.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(easymorse PRIVATE gfx/easymorse.icns)
    set_target_properties(easymorse PROPERTIES MACOSX_DEPLOYMENT_TARGET 12.0)
endif()

# Translations are compiled and embedded under :/i18n
qt_add_translations(easymorse
    TS_FILES intl/easymorse_fr_FR.ts
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
    install(FILES packaging/linux/easymorse.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES gfx/Morse_256x256.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME easymorse.png)
endif()

install(FILES LICENSE.GPLv3 README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

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

# =======================================================================
# Packaging

include(cmake/Packaging.cmake)
