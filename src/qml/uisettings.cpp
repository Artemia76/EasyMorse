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
* uisettings.cpp is part of EasyMorse project
* **************************************************************************/

#include "uisettings.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlEngine>

#include "tools/clogger.h"

UiSettings::UiSettings(QObject* parent)
    : QObject(parent)
{
    m_theme = static_cast<Theme>(qBound(0, m_settings.value("ui/Theme", 0).toInt(), 2));
    m_language = m_settings.value("ui/Language").toString();
    m_logMaxLines = qMax(10, m_settings.value("Log/MaxLine", 200).toInt());
}

UiSettings* UiSettings::create(QQmlEngine*, QJSEngine*)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void UiSettings::installTranslators()
{
    QCoreApplication::removeTranslator(&m_qtTranslator);
    QCoreApplication::removeTranslator(&m_appTranslator);
    const QLocale locale = m_language.isEmpty() ? QLocale() : QLocale(m_language);
    // Qt own translations (standard buttons, dialogs)
    if (m_qtTranslator.load(locale, "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&m_qtTranslator);
    // EasyMorse translations are embedded in resources
    if (m_appTranslator.load(locale, "easymorse", "_", ":/i18n"))
        QCoreApplication::installTranslator(&m_appTranslator);
    if (m_engine)
        m_engine->retranslate();
}

void UiSettings::setTheme(Theme pTheme)
{
    if (pTheme == m_theme)
        return;
    m_theme = pTheme;
    m_settings.setValue("ui/Theme", static_cast<int>(pTheme));
    emit themeChanged();
}

void UiSettings::setLanguage(const QString& pLanguage)
{
    if (pLanguage == m_language)
        return;
    m_language = pLanguage;
    m_settings.setValue("ui/Language", pLanguage);
    installTranslators();
    emit languageChanged();
}

QVariantList UiSettings::languages() const
{
    // Language names are written in their own language
    return {
        QVariantMap{{"code", QString()}, {"name", tr("System language")}},
        QVariantMap{{"code", "en"}, {"name", "English"}},
        QVariantMap{{"code", "fr_FR"}, {"name", "Français"}},
    };
}

int UiSettings::logLevel() const
{
    return static_cast<int>(CLogger::instance()->DebugLevel);
}

void UiSettings::setLogLevel(int pLevel)
{
    if (pLevel == logLevel())
        return;
    CLogger::instance()->setDebugLevel(static_cast<CL_DEBUG_LEVEL>(pLevel));
    emit logLevelChanged();
}

QString UiSettings::logFilter() const
{
    return CLogger::instance()->getFilter();
}

void UiSettings::setLogFilter(const QString& pFilter)
{
    if (pFilter == logFilter())
        return;
    CLogger::instance()->setFilter(pFilter);
    emit logFilterChanged();
}

void UiSettings::setLogMaxLines(int pLines)
{
    pLines = qMax(10, pLines);
    if (pLines == m_logMaxLines)
        return;
    m_logMaxLines = pLines;
    m_settings.setValue("Log/MaxLine", pLines);
    emit logMaxLinesChanged();
}
