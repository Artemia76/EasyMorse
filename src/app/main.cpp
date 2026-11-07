/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
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
* Entry point of the QML user interface
* **************************************************************************/

#include <QCommandLineParser>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

#include <cstdio>

#include "keyboardkeyer.h"
#include "logmodel.h"
#include "qmltypes.h"
#include "tools/clogger.h"
#include "uisettings.h"
#include "version.h"

namespace
{
    int s_qmlWarnings = 0;
    QtMessageHandler s_defaultHandler = nullptr;

    ///
    /// \brief counts the QML warnings for the self test, then logs as usual
    ///
    void countQmlWarnings(QtMsgType pType, const QMessageLogContext& pContext, const QString& pMessage)
    {
        const bool fromQml = pMessage.contains(".qml") || QByteArray(pContext.category ? pContext.category : "").startsWith("qt.qml");
        if (pType >= QtWarningMsg && fromQml)
            ++s_qmlWarnings;
        s_defaultHandler(pType, pContext, pMessage);
    }

    ///
    /// \brief SelfTest shows every page at desktop and phone sizes, optionally
    /// saving screenshots, then quits with the number of QML warnings
    ///
    class SelfTest : public QObject
    {
    public:
        SelfTest(QQuickWindow* pWindow, const QString& pScreenshotDir)
            : QObject(pWindow)
            , m_window(pWindow)
            , m_screenshotDir(pScreenshotDir)
        {
            const int pages = pWindow->property("pageCount").toInt();
            for (const auto& [width, height] : {std::pair{1100, 720}, std::pair{400, 820}})
            {
                // Phones have an extra page for the sound and speed controls
                for (int page = 0; page < pages + (width < 800 ? 1 : 0); ++page)
                    m_steps.append({width, height, page});
            }
            QTimer::singleShot(500, this, [this] { next(); });
        }

    private:
        struct Step
        {
            int width;
            int height;
            int page;
        };

        void next()
        {
            if (m_index > 0 && !m_screenshotDir.isEmpty())
            {
                const Step& shown = m_steps.at(m_index - 1);
                m_window->grabWindow().save(QDir(m_screenshotDir).filePath(
                    QString("%1-page%2.png").arg(shown.width < 800 ? "phone" : "desktop").arg(shown.page)));
            }
            if (m_index == m_steps.size())
            {
                std::printf("self test: %d QML warning(s)\n", s_qmlWarnings);
                QCoreApplication::exit(s_qmlWarnings);
                return;
            }
            const Step& step = m_steps.at(m_index++);
            m_window->resize(step.width, step.height);
            m_window->setProperty("currentPage", step.page);
            QTimer::singleShot(400, this, [this] { next(); });
        }

        QQuickWindow*   m_window;
        QString         m_screenshotDir;
        QList<Step>     m_steps;
        qsizetype       m_index = 0;
    };
}

int main(int argc, char *argv[])
{
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    QCoreApplication::setOrganizationName("EasyMorse");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("EasyMorse");
    QCoreApplication::setApplicationVersion(VER_PRODUCTVERSION_STR);
    QGuiApplication app(argc, argv);
    QGuiApplication::setWindowIcon(QIcon(":/gfx/Morse_128x128.png"));
    QQuickStyle::setStyle("Material");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption selfTestOption("self-test", "Show every page, then quit with the number of QML warnings.");
    const QCommandLineOption screenshotOption("screenshots", "With --self-test, save screenshots to <dir>.", "dir");
    parser.addOptions({selfTestOption, screenshotOption});
    parser.process(app);
    const bool selfTesting = parser.isSet(selfTestOption);
    if (selfTesting)
        s_defaultHandler = qInstallMessageHandler(countQmlWarnings);

    // Interface (Inter) and Morse text (JetBrains Mono) fonts, free (SIL OFL)
    for (const QString& font : QDir(":/fonts").entryList({"*.ttf"}))
        QFontDatabase::addApplicationFont(":/fonts/" + font);

    LogModel logModel;
    LogModel::s_instance = &logModel;
    UiSettings uiSettings;
    UiSettings::s_instance = &uiSettings;
    uiSettings.installTranslators();
    logModel.setMaxLines(uiSettings.logMaxLines());

    Settings settings;
    Trainer trainer(&settings);
    TrainerForeign::s_instance = &trainer;
    SettingsForeign::s_instance = &settings;
    KeyboardKeyer keyboardKeyer(trainer.manualKeyer());
    trainer.addPaddles(keyboardKeyer.paddles());

    QQmlApplicationEngine engine;
    uiSettings.setEngine(&engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("EasyMorse", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    CLogger::instance()->log(QCoreApplication::translate("main", "Welcome to EasyMorse %1").arg(VER_PRODUCTVERSION_STR), Qt::blue);
    // Sound card and serial key chosen in the options
    trainer.start();

    if (selfTesting)
        new SelfTest(qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst()), parser.value(screenshotOption));
    return app.exec();
}
