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
* jitterbuffer.h is part of EasyMorse project
*
* Plays back the key events of a remote station with their original rhythm:
* packets arrive with a variable delay, events are played at
* remote time + clock offset + a fixed delay.
* **************************************************************************/

#ifndef JITTERBUFFER_H
#define JITTERBUFFER_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QTimer>

#include "net/protocol.h"

class JitterBuffer : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultDelayMs = 200;

    explicit JitterBuffer(QObject* parent = nullptr);

    ///
    /// \brief delay added to the fastest packet seen: larger absorbs more network jitter
    ///
    void    setDelay(int pMs) { m_delayMs = qMax(0, pMs); }
    int     delay() const { return m_delayMs; }

    ///
    /// \brief when on (default) an internal timer plays the events in real
    /// time; tests turn it off and call advanceTo() themselves
    ///
    void    setAutoTimer(bool pAutoTimer);

    void    reset();

    ///
    /// \brief push events received at pArrivalMs (MonotonicClock), duplicates are ignored
    ///
    void    push(const QList<Protocol::KeyEvent>& pEvents, qint64 pArrivalMs);
    ///
    /// \brief play the events due at pNowMs
    ///
    void    advanceTo(qint64 pNowMs);

signals:
    ///
    /// \param pTimeMs local time (MonotonicClock) the event is played at
    ///
    void    keyChanged(bool pDown, qint64 pTimeMs);

private:
    qint64  playoutTime(const Protocol::KeyEvent& pEvent) const;
    void    armTimer();

    QMap<quint32, Protocol::KeyEvent>   m_pending;      // by sequence
    QList<qint64>                       m_offsets;      // recent arrival - remote time
    qint64                              m_targetOffset = 0; // smallest recent offset
    qint64                              m_offset = 0;   // offset in use, moved toward the target
    quint32                             m_adjustedFor = 0;  // key down that last moved the offset
    quint32                             m_nextSequence = 0;
    bool                                m_started = false;
    int                                 m_delayMs = DefaultDelayMs;
    bool                                m_autoTimer = true;
    QTimer                              m_timer;
};

#endif // JITTERBUFFER_H
