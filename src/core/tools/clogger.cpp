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
** clogger.cpp is part of EasyMorse project
**
** Singleton gathering the messages shown in the log page: they go through
** when they match the filter and the debug level.
****************************************************************************/

#include "clogger.h"

#include <QMutexLocker>

CLogger::CLogger(QObject* pParent)
    : QObject(pParent)
{
    qRegisterMetaType<CL_DEBUG_LEVEL>("CL_DEBUG_LEVEL");
    m_debugLevel = static_cast<CL_DEBUG_LEVEL>(qBound<int>(LEVEL_NONE, m_settings.value("Log/DebugLevel", LEVEL_NONE).toInt(), LEVEL_VERBOSE));
    m_filter = m_settings.value("Log/FiltreLog", "*").toString();
    m_rx.setPattern(QRegularExpression::wildcardToRegularExpression(m_filter));
}

CLogger* CLogger::instance()
{
    // Initialisation of a function static is thread-safe since C++11
    static CLogger logger;
    return &logger;
}

CL_DEBUG_LEVEL CLogger::debugLevel() const
{
    QMutexLocker lock(&m_mutex);
    return m_debugLevel;
}

void CLogger::setDebugLevel(CL_DEBUG_LEVEL pLevel)
{
    QMutexLocker lock(&m_mutex);
    m_debugLevel = static_cast<CL_DEBUG_LEVEL>(qBound<int>(LEVEL_NONE, pLevel, LEVEL_VERBOSE));
    m_settings.setValue("Log/DebugLevel", static_cast<int>(m_debugLevel));
}

QString CLogger::filter() const
{
    QMutexLocker lock(&m_mutex);
    return m_filter;
}

void CLogger::setFilter(const QString& pFilter)
{
    QMutexLocker lock(&m_mutex);
    m_filter = pFilter;
    m_rx.setPattern(QRegularExpression::wildcardToRegularExpression(m_filter));
    m_settings.setValue("Log/FiltreLog", m_filter);
}

void CLogger::log(const QString& pMessage, QColor pColor, CL_DEBUG_LEVEL pLevel)
{
    QMutexLocker lock(&m_mutex);
    if (pLevel == LEVEL_NONE || (pLevel <= m_debugLevel && m_rx.match(pMessage).hasMatch()))
        emit fireLog(pMessage, pColor, pLevel);
}
