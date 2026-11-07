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

# Phones and tablets: the application only, packaged by Qt (APK, Xcode)
if(ANDROID OR IOS)
    set(EASYMORSE_MOBILE ON)
else()
    set(EASYMORSE_MOBILE OFF)
endif()

if(EASYMORSE_MOBILE)
    set(_deploy_qt_default OFF)
elseif(WIN32 OR APPLE)
    set(_deploy_qt_default ON)
else()
    # Linux distributions provide Qt themselves
    set(_deploy_qt_default OFF)
endif()
option(EASYMORSE_DEPLOY_QT "Bundle Qt libraries and plugins when installing" ${_deploy_qt_default})
option(EASYMORSE_BUILD_APP "Build the EasyMorse application" ON)
if(EASYMORSE_MOBILE)
    set(_desktop_only OFF)
else()
    set(_desktop_only ON)
endif()
option(EASYMORSE_BUILD_TESTS "Build the unit tests" ${_desktop_only})
option(EASYMORSE_BUILD_SERVER "Build the match-making server (needs Qt HttpServer, WebSockets, Sql)" ${_desktop_only})

# =======================================================================
# Dependencies

find_package(Qt6 6.7 REQUIRED COMPONENTS Core)
if(EASYMORSE_BUILD_APP)
    find_package(Qt6 REQUIRED COMPONENTS Gui Network WebSockets Qml Quick QuickControls2 Svg LinguistTools
                        OPTIONAL_COMPONENTS SerialPort)
    if(NOT TARGET Qt6::SerialPort)
        message(STATUS "Qt Serial Port not found: serial port keys disabled")
    endif()
endif()
if(EASYMORSE_BUILD_SERVER)
    find_package(Qt6 OPTIONAL_COMPONENTS Network HttpServer WebSockets Sql)
    if(NOT (TARGET Qt6::HttpServer AND TARGET Qt6::WebSockets AND TARGET Qt6::Sql))
        message(STATUS "Qt HttpServer, WebSockets or Sql not found: server not built")
        set(EASYMORSE_BUILD_SERVER OFF)
    elseif(Qt6_VERSION VERSION_LESS 6.8)
        # WebSocket upgrades in QHttpServer came with Qt 6.8
        message(STATUS "The server needs Qt 6.8 or newer: server not built")
        set(EASYMORSE_BUILD_SERVER OFF)
    endif()
endif()
if(NOT EASYMORSE_BUILD_APP AND NOT EASYMORSE_BUILD_SERVER)
    message(FATAL_ERROR "Nothing to build: enable EASYMORSE_BUILD_APP or EASYMORSE_BUILD_SERVER")
endif()
qt_standard_project_setup(REQUIRES 6.7 I18N_SOURCE_LANGUAGE en I18N_TRANSLATED_LANGUAGES fr_FR)
set(CMAKE_AUTORCC ON)

if(EASYMORSE_BUILD_APP)
    add_subdirectory(3rdparty/miniaudio)
endif()
add_subdirectory(3rdparty/monocypher)
include(cmake/LibJuice.cmake)

# =======================================================================
# Common compile settings

add_library(easymorse_settings INTERFACE)
target_compile_definitions(easymorse_settings INTERFACE
    QT_DISABLE_DEPRECATED_UP_TO=0x060700
)
if(MSVC)
    target_compile_options(easymorse_settings INTERFACE /W4 /utf-8 /permissive-)
else()
    target_compile_options(easymorse_settings INTERFACE -Wall -Wextra)
endif()

# =======================================================================
# Version

set(EASYMORSE_GIT_REVISION "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE EASYMORSE_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(EASYMORSE_BUILD_APP)
    configure_file(src/app/version.h.in "${PROJECT_BINARY_DIR}/generated/version.h" @ONLY)
endif()

# =======================================================================
# Components

add_subdirectory(src/protocol) # messages shared with the server
if(EASYMORSE_BUILD_APP)
    add_subdirectory(src/core)  # audio, Morse logic, settings: no user interface
    add_subdirectory(src/app)   # QML application (desktop and mobile)
endif()
if(EASYMORSE_BUILD_SERVER)
    add_subdirectory(server)    # match-making server
endif()

if(EASYMORSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =======================================================================
# Packaging

if(EASYMORSE_BUILD_APP AND NOT EASYMORSE_MOBILE)
    include(cmake/Packaging.cmake)
endif()
