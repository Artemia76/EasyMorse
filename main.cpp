/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
** Contact: https://bitbucket.org/Artemia/easymorse/src/master/
**
** FFS2Play is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** FFS2Play is distributed in the hope that it will be useful,
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

int main(int argc, char *argv[])
{
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    QCoreApplication::setOrganizationName("EasyMorse");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("EasyMorse");
    QString locale = QLocale::system().name();
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    QTranslator EasyMorseTS;
#ifdef Q_OS_LINUX
    EasyMorseTS.load("/usr/share/easymorse/translations/easymorse_" + locale +".qm");
    QFile styleFile ("/usr/share/easymorse/easymorse.qss");
#else
#ifdef Q_OS_MACOS
    EasyMorseTS.load("../Resources/easymorse_"+ locale +".qm");
    QFile styleFile ("../Resources/easymorse.qss");
#else
    EasyMorseTS.load("translations/easymorse_" + locale +".qm");
    QFile styleFile ( "easymorse.qss");
#endif
#endif
    a.installTranslator(&EasyMorseTS);
    //Load Style Sheet
    styleFile.open(QFile::ReadOnly);
    if (styleFile.isOpen())
    {
        a.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
    MainWindow w;
    w.show();
    return a.exec();
}
