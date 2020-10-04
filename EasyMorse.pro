QT       += core gui multimedia serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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

# Application Version
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_BUILD = 0
DEFINES += APP_NAME=\\\"$${TARGET}\\\"
DEFINES += APP_MAJOR=$$VERSION_MAJOR
DEFINES += APP_MINOR=$$VERSION_MINOR
DEFINES += APP_BUILD=$$VERSION_BUILD

QMAKE_EXTRA_TARGETS += versionTarget

DEFINES += GIT_REVISION=$$GIT_REVISION

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
    main.cpp \
    gui/mainwindow.cpp \
    morse/analyze.cpp \
    morse/morse.cpp \
    sound/generator.cpp \
    tools/clogger.cpp \
    tools/datetime.cpp

HEADERS += \
    gui/mainwindow.h \
    morse/analyze.h \
    morse/morse.h \
    sound/generator.h \
    tools/call_once.h \
    tools/clogger.h \
    tools/datetime.h \
    tools/singleton.h \
    tools/version.h

FORMS += \
    gui/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

ICON = EasyMorse.icns

win32 : RC_FILE = EasyMorse.rc

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

# =====================================================================
# Additional targets

# Need to copy data when compiling
all.depends = copydata

# Deploy needs compiling before
deploy.depends = all

QMAKE_EXTRA_TARGETS += deploy copydata all
