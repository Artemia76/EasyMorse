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
** clogger.h is part of EasyMorse project
**
** Singleton gathering the messages shown in the log page: they go through
** when they match the filter and the debug level.
****************************************************************************/

#ifndef CLOGGER_H
#define CLOGGER_H

#include <QColor>
#include <QMutex>
#include <QObject>
#include <QRegularExpression>
#include <QSettings>

enum CL_DEBUG_LEVEL
{
    LEVEL_NONE,
    LEVEL_NORMAL,
    LEVEL_VERBOSE
};

Q_DECLARE_METATYPE(CL_DEBUG_LEVEL);

class CLogger : public QObject
{
    Q_OBJECT

public:
    static CLogger*         instance();

    CL_DEBUG_LEVEL          debugLevel() const;
    void                    setDebugLevel(CL_DEBUG_LEVEL pLevel);
    QString                 filter() const;
    ///
    /// \brief setFilter keeps only the messages matching a wildcard pattern ("*" for all)
    ///
    void                    setFilter(const QString& pFilter);
    ///
    /// \brief log a message; LEVEL_NONE messages always go through
    ///
    void                    log(const QString& pMessage, QColor pColor = Qt::black, CL_DEBUG_LEVEL pLevel = LEVEL_NONE);

signals:
    void                    fireLog(const QString& pMessage, QColor pColor, CL_DEBUG_LEVEL pLevel);

private:
    explicit                CLogger(QObject* pParent = nullptr);

    mutable QMutex          m_mutex;
    QSettings               m_settings;
    CL_DEBUG_LEVEL          m_debugLevel;
    QString                 m_filter;
    QRegularExpression      m_rx;
};

#endif // CLOGGER_H
