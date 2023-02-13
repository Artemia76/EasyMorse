#****************************************************************************
#
# Copyright (C) 2020 Gianni Peschiutta (F4IKZ)
# Contact: neophile76@hotmail.fr
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

# Application Version
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_BUILD = 4
DEFINES += APP_NAME=\\\"$${TARGET}\\\"
DEFINES += APP_MAJOR=$$VERSION_MAJOR
DEFINES += APP_MINOR=$$VERSION_MINOR
DEFINES += APP_BUILD=$$VERSION_BUILD

QMAKE_EXTRA_TARGETS += versionTarget

DEFINES += GIT_REVISION=$$GIT_REVISION

QT += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += build_all c++17
CONFIG -= debug_and_release debug_and_release_target

TARGET = easymorse
TARGET_NAME = $$TARGET
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TRANSLATIONS= \
    intl/easymorse_fr_FR.ts

# =======================================================================
# Copy ennvironment variables into qmake variables
DEPLOY_BASE=$$(DEPLOY_BASE)
GIT_PATH=$$(GIT_PATH)

isEmpty(DEPLOY_BASE) : DEPLOY_BASE=$$PWD/../deploy

# =======================================================================
# Set compiler flags and paths

unix:!macx {
    isEmpty(GIT_PATH) : GIT_PATH=git

    # Makes the shell script and setting LD_LIBRARY_PATH redundant
    QMAKE_RPATHDIR=.
    QMAKE_RPATHDIR+=./lib

    CONFIG += link_pkgconfig
    PKGCONFIG += portaudio-2.0
}

win32 {
    WINDEPLOY_FLAGS = --compiler-runtime --no-system-d3d-compiler --no-opengl --no-opengl-sw --no-angle

    CONFIG(debug, debug|release) : WINDEPLOY_FLAGS += --debug
    CONFIG(release, debug|release) : WINDEPLOY_FLAGS += --release

    DEFINES += _WINSOCKAPI_
    LIBS += -lWs2_32
    LIBS += -liphlpapi
    LIBS += -luser32
    LIBS += -lole32
}

macx {

    # Mac Specific
    # Compatibility down to OS X 10.10
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKG_CONFIG = /usr/local/bin/pkg-config

    #add /usr/local scope which is not included by default on osx
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
    LIBS += -framework CoreFoundation

    isEmpty(GIT_PATH) : GIT_PATH=git
}

isEmpty(GIT_PATH) {
  GIT_REVISION='\\"UNKNOWN\\"'
} else {
  GIT_REVISION='\\"$$system('$$GIT_PATH' rev-parse --short HEAD)\\"'
}

# =======================================================================
# Print values when running qmake
!isEqual(QUIET, "true") {
message(-----------------------------------)
message(GIT_PATH: $$GIT_PATH)
message(GIT_REVISION: $$GIT_REVISION)
message(DEPLOY_BASE: $$DEPLOY_BASE)
message(DEFINES: $$DEFINES)
message(INCLUDEPATH: $$INCLUDEPATH)
message(LIBS: $$LIBS)
message(TARGET_NAME: $$TARGET_NAME)
message(QT_INSTALL_PREFIX: $$[QT_INSTALL_PREFIX])
message(QT_INSTALL_LIBS: $$[QT_INSTALL_LIBS])
message(QT_INSTALL_PLUGINS: $$[QT_INSTALL_PLUGINS])
message(QT_INSTALL_TRANSLATIONS: $$[QT_INSTALL_TRANSLATIONS])
message(QT_INSTALL_BINS: $$[QT_INSTALL_BINS])
message(CONFIG: $$CONFIG)
message(-----------------------------------)
}

# =====================================================================
# Files

SOURCES += \
    audio/Audio.cpp \
    audio/Saw.cpp \
    audio/Sine.cpp \
    audio/Square.cpp \
    audio/Triangle.cpp \
    audio/envelope/Envelope.cpp \
    audio/filter/BandpassFilter.cpp \
    audio/filter/HighpassFilter.cpp \
    audio/filter/LowpassFilter.cpp \
    audio/modulation/ModulationValue.cpp \
    audio/oscillator/Lfo.cpp \
    audio/voice/Voice.cpp \
    audio/voicemanager/Param.cpp \
    audio/voicemanager/VoiceManager.cpp \
    gui/options.cpp \
    main.cpp \
    gui/mainwindow.cpp \
    morse/analyze.cpp \
    morse/morse.cpp \
    tools/clogger.cpp \
    tools/datetime.cpp

HEADERS += \
    audio/Audio.h \
    audio/DefaultParameters.h \
    audio/IOscillatorFunction.h \
    audio/envelope/Envelope.h \
    audio/envelope/IEnvelope.h \
    audio/envelope/NoEnvelope.h \
    audio/filter/BandpassFilter.h \
    audio/filter/FilterTypes.h \
    audio/filter/HighpassFilter.h \
    audio/filter/IFilter.h \
    audio/filter/LowpassFilter.h \
    audio/filter/NoFilter.h \
    audio/modulation/ModulationValue.h \
    audio/oscillator/ILfo.h \
    audio/oscillator/Lfo.h \
    audio/oscillator/NoOscillator.h \
    audio/voice/IVoice.h \
    audio/voice/Voice.h \
    audio/voicemanager/Param.h \
    audio/voicemanager/VoiceManager.h \
    gui/mainwindow.h \
    gui/options.h \
    morse/analyze.h \
    morse/morse.h \
    tools/call_once.h \
    tools/clogger.h \
    tools/datetime.h \
    tools/singleton.h \
    tools/version.h

FORMS += \
    gui/mainwindow.ui \
    gui/options.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

ICON = EasyMorse.icns

win32 : RC_FILE = EasyMorse.rc

OTHER_FILES += \
    README.md \
    changelog.txt
win32 {
    OTHER_FILES += easymorse.iss
}

# =====================================================================
# Local deployment commands for development
unix:!macx {
    copydata.commands += mkdir -p $$OUT_PWD/translations &&
    copydata.commands += cp -avfu $$PWD/intl/*.qm $$OUT_PWD/translations
}

# Mac OS X - Copy help and Marble plugins and data
macx {
    copydata.commands += cp -vf $$PWD/intl/*.qm $$OUT_PWD/easymorse.app/Contents/Resources &&
    copydata.commands += cp -vf $$PWD/fonts/*.ttf $$OUT_PWD/easymorse.app/Contents/Resources/fonts
}

#Windows - Copy Qt Style and translation
win32 {
    defineReplace(p){return ($$shell_quote($$shell_path($$1)))}
    exists($$OUT_PWD/translations) {
        message("existing")
    } else {
        copydata.commands = mkdir $$p($$OUT_PWD/translations) &&
    }
    copydata.commands += xcopy /Y $$p($$PWD/intl/*.qm) $$p($$OUT_PWD/translations)
}

# =====================================================================
# Deployment commands

#linux make install
unix:!macx {
    PREFIX = /usr
    Binary.path = $${PREFIX}/bin
    Binary.files = $$OUT_PWD/$${TARGET}
    INSTALLS += Binary

    Translation.path = $${PREFIX}/share/$${TARGET}/translations
    Translation.files = $$PWD/translations/*.qm
    INSTALLS += Translation

    Icons.path = $${PREFIX}/share/icons/hicolor/72x72/apps
    Icons.files = $$PWD/gfx/Morse_64x64.png
    INSTALLS += Icons

    Shortcut.path = $${PREFIX}/share/applications
    Shortcut.files = $$PWD/*.desktop
    INSTALLS += Shortcut
}

# Linux specific deploy target
unix:!macx {
    DEPLOY_DIR=\"$$DEPLOY_BASE/$$TARGET_NAME\"
    DEPLOY_DIR_LIB=\"$$DEPLOY_BASE/$$TARGET_NAME/lib\"
    message(-----------------------------------)
    message(DEPLOY_DIR: $$DEPLOY_DIR)
    message(-----------------------------------)
    deploy.commands += rm -Rfv $$DEPLOY_DIR &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/iconengines &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/platformthemes &&
    deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/printsupport &&
    deploy.commands += cp -Rvf $$OUT_PWD/translations $$DEPLOY_DIR &&
    deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qt_??.qm  $$DEPLOY_DIR/translations &&
    deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qt_??_??.qm  $$DEPLOY_DIR/translations &&
    deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qtbase*.qm  $$DEPLOY_DIR/translations &&
    deploy.commands += cp -vf $$PWD/desktop/qt.conf $$DEPLOY_DIR &&
    deploy.commands += cp -vf $$PWD/changelog.txt $$DEPLOY_DIR &&
    deploy.commands += cp -vf $$PWD/README.md $$DEPLOY_DIR &&
    deploy.commands += cp -vf $$PWD/LICENSE.GPLv3 $$DEPLOY_DIR &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/iconengines/libqsvgicon.so*  $$DEPLOY_DIR_LIB/iconengines &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqgif.so*  $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqjpeg.so*  $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqsvg.so*  $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqwbmp.so*  $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqwebp.so*  $$DEPLOY_DIR_LIB/imageformats &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqeglfs.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqlinuxfb.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqminimal.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqminimalegl.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqoffscreen.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqxcb.so*  $$DEPLOY_DIR_LIB/platforms &&
    deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platformthemes/libqgtk*.so*  $$DEPLOY_DIR_LIB/platformthemes &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicudata.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicui18n.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicuuc.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Concurrent.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Core.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5DBus.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Gui.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Network.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Qml.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Quick.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Script.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Sql.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Svg.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Widgets.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5X11Extras.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5XcbQpa.so*  $$DEPLOY_DIR_LIB &&
    deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Xml.so* $$DEPLOY_DIR_LIB
}

# Mac specific deploy target
macx {
    DEPLOY_DIR=\"$$PWD/../deploy\"
    message(-----------------------------------)
    message(DEPLOY_DIR: $$DEPLOY_DIR)
    message(-----------------------------------)
    deploy.commands += rm -Rfv $$DEPLOY_DIR/* &&
    !exists($$DEPLOY_DIR) : deploy.commands += mkdir -p $$DEPLOY_DIR &&
    deploy.commands += mkdir -p $$OUT_PWD/EasyMorse.app/Contents/PlugIns &&
    deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qt_??.qm  $$OUT_PWD/EasyMorse.app/Contents/Resources &&
    deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qt_??_??.qm  $$OUT_PWD/EasyMorse.app/Contents/Resources &&
    deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qtbase*.qm  $$OUT_PWD/EasyMorse.app/Contents/Resources &&
    deploy.commands += $$[QT_INSTALL_PREFIX]/bin/macdeployqt EasyMorse.app -always-overwrite -dmg &&
    deploy.commands += cp -fv $$OUT_PWD/EasyMorse.dmg $$DEPLOY_DIR/EasyMorse_$${VERSION_MAJOR}_$${VERSION_MINOR}_$${VERSION_BUILD}_mac.dmg
}

# Windows specific deploy target
win32 {
    defineReplace(p){return ($$shell_quote($$shell_path($$1)))}
    RC_ICONS = $$p($$PWD/gfx/Morse.ico)
    CONFIG(debug, debug|release) : DLL_SUFFIX=d
    CONFIG(release, debug|release) : DLL_SUFFIX=
    deploy.commands = ( rmdir /s /q $$p($$DEPLOY_BASE) || echo Directory already empty) &&
    deploy.commands += mkdir $$p($$DEPLOY_BASE/$$TARGET_NAME/translations) &&
    deploy.commands += xcopy /Y $$p($$OUT_PWD/$$TARGET.$$TARGET_EXT) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += xcopy /Y $$p($$PWD/README.md) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += xcopy /Y $$p($$PWD/intl/*.qm) $$p($$DEPLOY_BASE/$$TARGET_NAME/translations) &&
    #deploy.commands += xcopy /Y $$p($$PWD/*.txt) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += xcopy /Y $$p($$PWD/LICENSE.GPLv3) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += xcopy /Y $$p($$PWD/easymorse.iss) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += xcopy /Y $$p($$PWD/gfx/Morse.ico) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += $$p($$[QT_INSTALL_BINS]/windeployqt) $$WINDEPLOY_FLAGS $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
    deploy.commands += compil32 /cc $$p($$DEPLOY_BASE/$$TARGET_NAME/easymorse.iss)
}

# =====================================================================
# Additional targets

# Need to copy data when compiling
all.depends = copydata

# Deploy needs compiling before
deploy.depends = all

QMAKE_EXTRA_TARGETS += deploy copydata all

DISTFILES += \
    LICENSE.GPLv3
