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

CAnalyze::CAnalyze(QObject *parent)
    : QObject(parent)
{
    m_log = CLogger::instance();
#ifdef QT_DEBUG
    m_log->log("Morse Analyser started...",Qt::magenta,LEVEL_NORMAL);
#endif
    m_wordSilentDuration = 1000;
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timer()));
    m_timer.setSingleShot(true);
    m_timer.start(m_wordSilentDuration);
    m_lastTrigOff=0;
    m_lastTrigOn=0;
    m_settings.beginGroup("morse");
    m_dotDuration = m_settings.value("DotDuration",0).toULongLong();
    m_dashDuration = m_settings.value("DashDuration",0).toULongLong();
    m_symbSilentDuration = m_settings.value("SymbolSilentDuration",0).toULongLong();
    m_charSilentDuration = m_settings.value("CharacterSilentDuration",0).toULongLong();
    m_autoAdapt=m_settings.value("AutoAdapt",true).toBool();
    m_settings.endGroup();
}

CAnalyze::~CAnalyze()
{
    m_settings.beginGroup("morse");
    m_settings.setValue("DotDuration",m_dotDuration);
    m_settings.setValue("DashDuration",m_dashDuration);
    m_settings.setValue("SymbolSilentDuration",m_symbSilentDuration);
    m_settings.setValue("CharacterSilentDuration",m_charSilentDuration);
    m_settings.setValue("AutoAdapt",m_autoAdapt);
    m_settings.endGroup();
#ifdef QT_DEBUG
    m_log->log("Morse Analyser destroyed...",Qt::magenta,LEVEL_NORMAL);
#endif
}

///
/// \brief On_keyer is a slot called to record key
/// \param state
/// \details key state is recorded in buffer. The analyser timer is restarted based on inter word silent duration
///
void CAnalyze::on_keyer(bool state)
{
#ifdef QT_DEBUG
    m_log->log(QString("Morse Analyser keyer value = %1 ...").arg(state),Qt::magenta,LEVEL_VERBOSE);
#endif
    QByteArray buff;
    QDataStream ds(&buff,QIODevice::WriteOnly);
    ds << static_cast<quint64>(toTimeStamp(Now()));
    ds << state;
    m_buffer.append(buff);
    m_timer.start(m_wordSilentDuration);
}

///
/// \brief CAnalyze::on_timer
/// \details on_timer is triggered once word or sentence as been finished to analyse the morse code and send it over network and display
///
void CAnalyze::on_timer()
{
    if (m_buffer.length()==0)
    {
        m_timer.start(m_wordSilentDuration);
        return;
    }
    quint64 DotDuration=1000;
    quint64 DashDuration=0;
    quint64 SymbSilentDuration=1000;
    QDataStream ds(&m_buffer,QIODevice::ReadOnly);
#ifdef QT_DEBUG
    m_log->log(QString("Morse Analyser On Timer Triggered"),Qt::magenta,LEVEL_VERBOSE);
#endif
    while (!ds.atEnd())
    {
        quint64 TimeStamp;
        bool State;
        ds >> TimeStamp;
        ds >> State;
#ifdef QT_DEBUG
        m_log->log(QString("Time = %1 , Key = %2").arg(TimeStamp).arg(State),Qt::magenta,LEVEL_VERBOSE);
#endif
        if (!State && (m_lastTrigOff==0))
            m_lastTrigOff = TimeStamp;
        if (State && (m_lastTrigOn==0))
            m_lastTrigOn = TimeStamp;
        //if this the end of dash or dot
        if ((m_lastTrigOn < m_lastTrigOff) && (m_lastTrigOff!=0) && (m_lastTrigOn!=0))
        {
            quint64 Duration = m_lastTrigOff-m_lastTrigOn;
            //if auto adaptive is on, we analyse the dot, dash , symbol space and char space duration
            if (m_autoAdapt)
            {
                if (Duration < DotDuration) DotDuration = Duration;
                if (Duration > DashDuration) DashDuration = Duration;
            }
            m_lastTrigOn=0;
        }
        //if this is the end of a silent
        else if ((m_lastTrigOff < m_lastTrigOn) && (m_lastTrigOff!=0) && (m_lastTrigOn!=0))
        {
            quint64 Duration = m_lastTrigOn-m_lastTrigOff;
            //if auto adaptive is on, we analyse the dot, dash , symbol space and char space duration
            if (m_autoAdapt)
            {
                if (Duration < SymbSilentDuration) SymbSilentDuration = Duration;
            }
            m_lastTrigOff=0;
        }
    }
    if (m_autoAdapt)
    {
        if (m_dotDuration) m_dotDuration = (m_dotDuration + DotDuration)/2;
        else m_dotDuration = DotDuration;
        if (m_dashDuration) m_dashDuration = (m_dashDuration + DashDuration)/2;
        else m_dashDuration = DashDuration;
        if (SymbSilentDuration)
        {
            m_symbSilentDuration = (m_symbSilentDuration + SymbSilentDuration)/2;
            m_charSilentDuration = (m_charSilentDuration + (SymbSilentDuration*3))/2;
        }
        else
        {
            m_symbSilentDuration = m_dotDuration;
            m_charSilentDuration = m_dotDuration*3;
        }
        m_wordSilentDuration = m_charSilentDuration*2;
#ifdef QT_DEBUG
        m_log->log("Auto adaptive analyzer is on :",Qt::magenta,LEVEL_NORMAL);
        m_log->log(QString("Dot duration in msec : %1").arg(m_dotDuration),Qt::magenta,LEVEL_NORMAL);
        m_log->log(QString("Dash duration in msec : %1").arg(m_dashDuration),Qt::magenta,LEVEL_NORMAL);
        m_log->log(QString("Symbol Silent duration in msec : %1").arg(m_symbSilentDuration),Qt::magenta,LEVEL_NORMAL);
        m_log->log(QString("Char Silent duration in msec : %1").arg(m_charSilentDuration),Qt::magenta,LEVEL_NORMAL);
#endif
    }
    emit (fire_message(DecodeMorse()));

    m_lastString.clear();


    m_buffer.clear();
    m_buffer.resize(0);
    m_timer.start(m_wordSilentDuration);
}

///
/// \brief CAnalyze::DecodeMorse
///
QString CAnalyze::DecodeMorse()
{
    m_lastString.clear();
    quint64 LastTrigOn=0;
    quint64 LastTrigOff=0;
    quint64 DotDash = m_dotDuration + ((m_dashDuration-m_dotDuration)/2);
    quint64 SymbChar = m_symbSilentDuration + ((m_charSilentDuration-m_symbSilentDuration)/2);
#ifdef QT_DEBUG
    m_log->log(QString("DotDash Separation in msec : %1").arg(DotDash),Qt::magenta,LEVEL_NORMAL);
    m_log->log(QString("SymbChar Silent Separation in msec : %1").arg(SymbChar),Qt::magenta,LEVEL_NORMAL);
    m_log->log(QString("WordDuration Silent duration in msec : %1").arg(m_wordSilentDuration),Qt::magenta,LEVEL_NORMAL);
#endif
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
                m_lastString.append('P');
            }
            else
            {
                m_lastString.append('T');
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
            else if ((Duration > SymbChar) && (Duration <m_wordSilentDuration))
            {
                m_lastString.append("/");
            }
            else
            // It's a word separation
            {
                m_lastString.append(" ");
            }
            LastTrigOff=0;
        }
    }
#ifdef QT_DEBUG
    m_log->log("Message is : "+ m_lastString);
#endif
    return m_lastString;
    //m_log->log(m_morse->decodeMorse(m_lastString));
}
