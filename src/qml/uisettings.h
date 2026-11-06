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
* uisettings.h is part of EasyMorse project
*
* Preferences of the user interface itself: theme, language, log display.
* Morse and device settings live in the core Settings object.
* **************************************************************************/

#ifndef UISETTINGS_H
#define UISETTINGS_H

#include <QObject>
#include <QSettings>
#include <QTranslator>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

class UiSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(Theme theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(int logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
    Q_PROPERTY(QString logFilter READ logFilter WRITE setLogFilter NOTIFY logFilterChanged)
    Q_PROPERTY(int logMaxLines READ logMaxLines WRITE setLogMaxLines NOTIFY logMaxLinesChanged)

public:
    enum class Theme
    {
        System,
        Light,
        Dark
    };
    Q_ENUM(Theme)

    explicit UiSettings(QObject* parent = nullptr);

    ///
    /// \brief installTranslators loads the chosen language; call again after a change
    ///
    void        installTranslators();
    void        setEngine(QQmlEngine* pEngine) { m_engine = pEngine; }

    Theme       theme() const { return m_theme; }
    void        setTheme(Theme pTheme);
    QString     language() const { return m_language; }
    void        setLanguage(const QString& pLanguage);
    QVariantList languages() const;
    int         logLevel() const;
    void        setLogLevel(int pLevel);
    QString     logFilter() const;
    void        setLogFilter(const QString& pFilter);
    int         logMaxLines() const { return m_logMaxLines; }
    void        setLogMaxLines(int pLines);

    // QML singleton backed by the instance created in main()
    inline static UiSettings* s_instance = nullptr;
    static UiSettings* create(QQmlEngine*, QJSEngine*);

signals:
    void        themeChanged();
    void        languageChanged();
    void        logLevelChanged();
    void        logFilterChanged();
    void        logMaxLinesChanged();

private:
    QSettings       m_settings;
    QTranslator     m_qtTranslator;
    QTranslator     m_appTranslator;
    QQmlEngine*     m_engine = nullptr;
    Theme           m_theme;
    QString         m_language;
    int             m_logMaxLines;
};

#endif // UISETTINGS_H
