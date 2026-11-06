# Locate PortAudio and expose it as EasyMorse::PortAudio.
#
# Lookup order:
#   1. CMake package (vcpkg, Conan, PortAudio installed with CMake)
#   2. pkg-config (Linux distributions, Homebrew, MSYS2)
#   3. Download and build a static PortAudio (always used when
#      EASYMORSE_FETCH_PORTAUDIO is ON)
#
# PortAudio is temporary: it will be replaced by miniaudio.

if(NOT EASYMORSE_FETCH_PORTAUDIO)
    find_package(portaudio CONFIG QUIET)
    if(TARGET portaudio_static)
        add_library(EasyMorse::PortAudio ALIAS portaudio_static)
    elseif(TARGET portaudio)
        add_library(EasyMorse::PortAudio ALIAS portaudio)
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PORTAUDIO IMPORTED_TARGET GLOBAL portaudio-2.0)
            if(PORTAUDIO_FOUND)
                add_library(EasyMorse::PortAudio ALIAS PkgConfig::PORTAUDIO)
            endif()
        endif()
    endif()
endif()

if(NOT TARGET EasyMorse::PortAudio)
    message(STATUS "PortAudio not found on the system, building it from source")
    include(FetchContent)
    set(PA_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(PA_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(PA_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    # PortAudio 19.7.0 still declares cmake_minimum_required(2.8)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG 147dd722548358763a8b649b3e4b41dfffbcfbb6 # v19.7.0
        GIT_SHALLOW FALSE
        SYSTEM
    )
    FetchContent_MakeAvailable(portaudio)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
    add_library(EasyMorse::PortAudio ALIAS portaudio_static)
endif()
