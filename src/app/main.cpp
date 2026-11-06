/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
** Contact: https://github.com/Artemia76/EasyMorse
**
** EasyMorse is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** EasyMorse is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** The license is as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

/****************************************************************************
 * main.cpp is part of EasyMorse project
 *
 * This is the entry point of easymorse application
 * **************************************************************************/

#include "gui/mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QTranslator>
#include <QDir>
#include <QLibraryInfo>

#include "audio/AudioEngine.h"

int main(int argc, char *argv[])
{
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    QCoreApplication::setOrganizationName("EasyMorse");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("EasyMorse");
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    // Qt own translations (standard dialogs, buttons...)
    QTranslator QtTS;
    if (QtTS.load(QLocale(), "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        a.installTranslator(&QtTS);
    // EasyMorse translations are embedded in resources
    QTranslator EasyMorseTS;
    if (EasyMorseTS.load(QLocale(), "easymorse", "_", ":/i18n"))
        a.installTranslator(&EasyMorseTS);
#ifdef Q_OS_LINUX
    QFile styleFile ("/usr/share/easymorse/easymorse.qss");
#else
#ifdef Q_OS_MACOS
    QFile styleFile ("../Resources/easymorse.qss");
#else
    QFile styleFile ( "easymorse.qss");
#endif
#endif
    //Load Style Sheet
    if (styleFile.open(QFile::ReadOnly))
    {
        a.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
    // The main window opens the sound card chosen in the options
    AudioEngine audio;
    MainWindow w(&audio);
    w.show();
    return a.exec();
}
