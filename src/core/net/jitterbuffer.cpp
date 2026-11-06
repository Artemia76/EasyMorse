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
* jitterbuffer.cpp is part of EasyMorse project
* **************************************************************************/

#include "jitterbuffer.h"

#include <algorithm>
#include <cmath>

#include "keyer/monotonicclock.h"

namespace
{
    // Packets used to estimate the clock offset; the window lets the
    // estimate follow a slow clock drift
    constexpr qsizetype OffsetWindow = 64;
    // The offset in use moves toward the estimate by this much at most, and
    // only at the start of an element: element lengths stay exact, only
    // silences absorb the correction
    constexpr qint64 MaxOffsetStepMs = 10;
}

JitterBuffer::JitterBuffer(QObject* parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { advanceTo(MonotonicClock::nowMs()); });
}

void JitterBuffer::setAutoTimer(bool pAutoTimer)
{
    m_autoTimer = pAutoTimer;
    armTimer();
}

void JitterBuffer::reset()
{
    m_pending.clear();
    m_offsets.clear();
    m_started = false;
    m_targetOffset = 0;
    m_offset = 0;
    m_timer.stop();
}

qint64 JitterBuffer::playoutTime(const Protocol::KeyEvent& pEvent) const
{
    return static_cast<qint64>(pEvent.timeMs) + m_offset + m_delayMs;
}

void JitterBuffer::push(const QList<Protocol::KeyEvent>& pEvents, qint64 pArrivalMs)
{
    for (const Protocol::KeyEvent& event : pEvents)
    {
        if (!m_started)
        {
            m_started = true;
            m_nextSequence = event.sequence;
            m_adjustedFor = event.sequence - 1;
            // First packet: take its offset at once
            m_offset = pArrivalMs - static_cast<qint64>(pEvents.last().timeMs);
        }
        // Already played, or a copy of a pending one
        if (static_cast<qint32>(event.sequence - m_nextSequence) < 0 || m_pending.contains(event.sequence))
            continue;
        m_pending.insert(event.sequence, event);
    }
    if (!pEvents.isEmpty())
    {
        // The newest event of the packet was sent just before it: its offset
        // is the network delay plus the clock difference. The smallest recent
        // one is the fastest path; the others were delayed by jitter.
        m_offsets.append(pArrivalMs - static_cast<qint64>(pEvents.last().timeMs));
        if (m_offsets.size() > OffsetWindow)
            m_offsets.removeFirst();
        m_targetOffset = *std::min_element(m_offsets.cbegin(), m_offsets.cend());
    }
    advanceTo(pArrivalMs);
}

void JitterBuffer::advanceTo(qint64 pNowMs)
{
    while (!m_pending.isEmpty())
    {
        const auto first = m_pending.cbegin();
        const Protocol::KeyEvent event = first.value();
        if (event.down && event.sequence != m_adjustedFor)
        {
            m_adjustedFor = event.sequence;
            m_offset += qBound(-MaxOffsetStepMs, m_targetOffset - m_offset, MaxOffsetStepMs);
        }
        const qint64 due = playoutTime(event);
        if (due > pNowMs)
            break;
        // A missing event is given up when the next one is due: the
        // redundancy of the following packets did not bring it
        m_pending.erase(first);
        m_nextSequence = event.sequence + 1;
        emit keyChanged(event.down, qMin(due, pNowMs));
    }
    armTimer();
}

void JitterBuffer::armTimer()
{
    if (!m_autoTimer || m_pending.isEmpty())
    {
        m_timer.stop();
        return;
    }
    const qint64 wait = playoutTime(m_pending.first()) - MonotonicClock::nowMs();
    m_timer.start(static_cast<int>(qBound(qint64(0), wait, qint64(10000))));
}
