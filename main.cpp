#include "mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QTranslator>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Artemia");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("EasyMorse");
    QString locale = QLocale::system().name();
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    MainWindow w;
    w.show();
    return a.exec();
}
