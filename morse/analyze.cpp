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
#include "morse.h"

CAnalyze::CAnalyze(QObject *parent, CMorse* pMorse)
    : QObject(parent)
{
    m_morse = pMorse;
    m_log = CLogger::instance();
#ifdef QT_DEBUG
    m_log->log("Morse Analyser started...",Qt::magenta,LEVEL_NORMAL);
#endif
    m_word_space_time = 1000;
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timer()));
    m_timer.setSingleShot(true);
    m_dash_duration=m_dot_duration=m_symb_silent=m_char_silent=0;
}

CAnalyze::~CAnalyze()
{
#ifdef QT_DEBUG
    m_log->log("Morse Analyser destroyed...",Qt::magenta,LEVEL_NORMAL);
#endif
}

void CAnalyze::on_keyer(bool state)
{
#ifdef QT_DEBUG
    m_log->log(QString("Morse Analyser keyer value = %1 ...").arg(state),Qt::magenta,LEVEL_NORMAL);
#endif
    QByteArray buff;
    QDataStream ds(&buff,QIODevice::WriteOnly);
    ds << static_cast<quint64>(toTimeStamp(Now()));
    ds << state;
    m_buffer.append(buff);
    m_timer.start(m_word_space_time);
}

void CAnalyze::on_timer()
{
    quint64 LastTrigOn=0;
    quint64 LastTrigOff=0;
    quint64 DotDuration=1000;
    quint64 DashDuration=0;
    quint64 SymbSilentDuration=1000;
    quint64 CharSilentDuration=0;
    QDataStream ds(&m_buffer,QIODevice::ReadOnly);
#ifdef QT_DEBUG
    m_log->log(QString("Morse Analyser On Timer Triggered"),Qt::magenta,LEVEL_NORMAL);
#endif
    while (!ds.atEnd())
    {
        quint64 TimeStamp;
        bool State;
        ds >> TimeStamp;
        ds >> State;
        m_log->log(QString("Time = %1 , Key = %2").arg(TimeStamp).arg(State),Qt::magenta,LEVEL_NORMAL);
        if (!State && (LastTrigOff==0))
            LastTrigOff = TimeStamp;
        if (State && (LastTrigOn==0))
            LastTrigOn = TimeStamp;
        //if this the end of dash or dot
        if ((LastTrigOn < LastTrigOff) && (LastTrigOff!=0) && (LastTrigOn!=0))
        {
            quint64 Duration = LastTrigOff-LastTrigOn;
            if (Duration < DotDuration) DotDuration = Duration;
            if (Duration > DashDuration) DashDuration = Duration;
            LastTrigOn=0;
        }
        //if this is the end of a silent
        else if ((LastTrigOff < LastTrigOn) && (LastTrigOff!=0) && (LastTrigOn!=0))
        {
            quint64 Duration = LastTrigOn-LastTrigOff;
            if (Duration < SymbSilentDuration) SymbSilentDuration = Duration;
            LastTrigOff=0;
        }
    }
    m_log->log("Stats:");
    if (m_dot_duration) m_dot_duration = (m_dot_duration + DotDuration)/2;
    else m_dot_duration = DotDuration;
    if (m_dash_duration) m_dash_duration = (m_dash_duration + DashDuration)/2;
    else m_dash_duration = DashDuration;
    if (m_symb_silent)  m_symb_silent = (m_symb_silent + SymbSilentDuration)/2;
    else m_symb_silent = SymbSilentDuration;
    if (m_char_silent)  m_char_silent = (m_char_silent + (SymbSilentDuration*3))/2;
    else m_char_silent = SymbSilentDuration*3;
    m_log->log(QString("Dot duration in msec : %1").arg(m_dot_duration));
    m_log->log(QString("Dash duration in msec : %1").arg(m_dash_duration));
    m_log->log(QString("Symbol Silent duration in msec : %1").arg(m_symb_silent));
    m_log->log(QString("Char Silent duration in msec : %1").arg(m_char_silent));
    DecodeMorse();

    m_last_String.clear();


    m_buffer.clear();
    m_buffer.resize(0);

}

void CAnalyze::DecodeMorse()
{
    m_last_String.clear();
    quint64 LastTrigOn=0;
    quint64 LastTrigOff=0;
    //Calculate Margin 10%
    quint64 DotDash = m_dot_duration + ((m_dash_duration-m_dot_duration)/2);
    quint64 SymbChar = m_symb_silent + ((m_char_silent-m_symb_silent)/2);
    quint64 WordDuration = m_char_silent*3;
    m_log->log(QString("DotDash Separation in msec : %1").arg(DotDash));
    m_log->log(QString("SymbChar Silent Separation in msec : %1").arg(SymbChar));
    m_log->log(QString("WordDuration Silent duration in msec : %1").arg(WordDuration));
    QDataStream ds(&m_buffer,QIODevice::ReadOnly);
    while (!ds.atEnd())
    {
        quint64 TimeStamp;
        bool State;
        ds >> TimeStamp;
        ds >> State;
        if (!State && (LastTrigOff==0))
            LastTrigOff = TimeStamp;
        if (State && (LastTrigOn==0))
            LastTrigOn = TimeStamp;
        //if this the end of dash or dot
        if ((LastTrigOn < LastTrigOff) && (LastTrigOff!=0) && (LastTrigOn!=0))
        {
            quint64 Duration = LastTrigOff-LastTrigOn;
            // Test if we have a dot
            if (Duration <= DotDash)
            {
                m_last_String.append('P');
            }
            else
            {
                m_last_String.append('T');
            }
            LastTrigOn=0;
        }
        //if this is the end of a silent
        else if ((LastTrigOff < LastTrigOn) && (LastTrigOff!=0) && (LastTrigOn!=0))
        {
            quint64 Duration = LastTrigOn-LastTrigOff;
            if (Duration <= SymbChar )
            {
               // Dash or Dot separation
            }
            // It's a char separation
            else if ((Duration > SymbChar) && (Duration <WordDuration))
            {
                m_last_String.append("/");
            }
            else
            // It's a word separation
            {
                m_last_String.append(" ");
            }
            LastTrigOff=0;
        }
    }
    m_log->log("Message is : "+ m_last_String);
    m_log->log(m_morse->decodeMorse(m_last_String));
}
