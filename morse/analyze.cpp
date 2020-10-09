/****************************************************************************
**
** Copyright (C) 2020 Gianni Peschiutta
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
** analyze.cpp is part of EasyMorse project
**
** This template provide safe thread calls for singletons
****************************************************************************/

#include <QApplication>

#include "analyze.h"


CAnalyze::CAnalyze(QObject *parent) : QObject(parent)
{
    m_log = CLogger::instance();
#ifdef QT_DEBUG
    m_log->log(tr("Morse Analyser started..."),Qt::magenta,LEVEL_NORMAL);
#endif
}

CAnalyze::~CAnalyze()
{
#ifdef QT_DEBUG
    m_log->log(tr("Morse Analyser destroyed..."),Qt::magenta,LEVEL_NORMAL);
#endif
}

void CAnalyze::on_keyer(bool state)
{
#ifdef QT_DEBUG
    m_log->log(QString(tr("Morse Analyser keyer value = %1 ...")).arg(state),Qt::magenta,LEVEL_NORMAL);
#endif
}
