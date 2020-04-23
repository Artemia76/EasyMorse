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
    m_keyPressed=false;
    m_log = CLogger::instance();
#ifdef QT_DEBUG
    m_log->log(tr("Morse Analyser started..."),Qt::magenta,LEVEL_NORMAL);
#endif
    qApp->installEventFilter(this);
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(onUpdate()));
    m_timer.start(5000);
}

CAnalyze::~CAnalyze()
{
#ifdef QT_DEBUG
    m_log->log(tr("Morse Analyser destroyed..."),Qt::magenta,LEVEL_NORMAL);
#endif
}

bool CAnalyze::eventFilter(QObject * Object, QEvent *Event)
{
    switch (Event->type())
    {
        case QEvent::Timer :
        {
            if (Object==&m_timer)
            {
                on_timer();
                return true;
            }
            break;
        }
        case QEvent::KeyPress:
        {
            keyPressEvent((QKeyEvent*)Event);
            return false;
        }
        case QEvent::KeyRelease:
        {
            keyReleaseEvent((QKeyEvent*)Event);
            return false;
        }
        default:
        {
        }
    }
    return false;
}

void CAnalyze::keyPressEvent(QKeyEvent* event)
{
    if ((event->key()==Qt::Key_Alt)&&(!m_keyPressed))
    {
#ifdef QT_DEBUG
        m_log->log(tr("Morse Analyser key pressed"),Qt::magenta,LEVEL_VERBOSE);
#endif
        m_keyPressed=true;
    }
}

void CAnalyze::keyReleaseEvent(QKeyEvent* event)
{
    if ((event->key()==Qt::Key_Alt)&&m_keyPressed)
    {
#ifdef QT_DEBUG
        m_log->log(tr("Morse Analyser key released"),Qt::magenta,LEVEL_VERBOSE);
#endif
        m_keyPressed=false;
    }
}

void CAnalyze::on_timer()
{
#ifdef QT_DEBUG
    m_log->log(tr("Analyze Timer Update"),Qt::magenta,LEVEL_VERBOSE);
#endif
}
