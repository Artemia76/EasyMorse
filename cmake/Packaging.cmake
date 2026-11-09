# CPack configuration: Inno Setup + ZIP on Windows, DMG on macOS,
# DEB + TGZ on Linux.

set(CPACK_PACKAGE_NAME "EasyMorse")
set(CPACK_PACKAGE_VENDOR "F4IKZ")
set(CPACK_PACKAGE_CONTACT "Gianni Peschiutta <neophile76@hotmail.fr>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "EasyMorse")
set(CPACK_PACKAGE_EXECUTABLES "easymorse;EasyMorse")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.GPLv3")
set(CPACK_STRIP_FILES ON)

if(WIN32)
    set(CPACK_SYSTEM_NAME "win64")
elseif(APPLE)
    set(CPACK_SYSTEM_NAME "macos")
else()
    set(CPACK_SYSTEM_NAME "linux-${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}" CPACK_PACKAGE_FILE_NAME)

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
        list(APPEND CPACK_GENERATOR "INNOSETUP")
    endif()
    # Same AppId as the former easymorse.iss so upgrades replace old installs
    set(CPACK_INNOSETUP_SETUP_AppId "{{DB764E94-E3B1-4475-8609-C0882210D89C}")
    set(CPACK_INNOSETUP_ARCHITECTURE x64)
    set(CPACK_INNOSETUP_ICON_FILE "${CMAKE_SOURCE_DIR}/gfx/Morse.ico")
    set(CPACK_INNOSETUP_LANGUAGES english french)
    set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
    set(CPACK_INNOSETUP_RUN_EXECUTABLES easymorse)
    set(CPACK_CREATE_DESKTOP_LINKS easymorse)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "EasyMorse")
    # Only the application in the disk image, not the bare server binary
    set(CPACK_COMPONENTS_ALL Unspecified)
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_SECTION "hamradio")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    if(EASYMORSE_DEPLOY_QT)
        # Qt is inside the packages: only what it needs from the system is
        # declared, with the names of both Debian's and Ubuntu's libraries
        set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/CPackLinuxBundle.cmake")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "libc6 (>= 2.34), libstdc++6 (>= 13.1), libgcc-s1, libgl1, libopengl0, libegl1, libglx0, libfontconfig1, libfreetype6, libdbus-1-3, libglib2.0-0t64 | libglib2.0-0, libx11-6, libx11-xcb1, libxcb1, libxkbcommon0, libxkbcommon-x11-0, libxcb-cursor0, libxcb-glx0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render0, libxcb-render-util0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-util1, libxcb-xfixes0, libxcb-xkb1")
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libssl3t64 | libssl3, libasound2t64 | libasound2")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_SOURCE_DIR}/packaging/linux/deb/postinst;${CMAKE_SOURCE_DIR}/packaging/linux/deb/prerm")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
    else()
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    endif()
endif()

include(CPack)
