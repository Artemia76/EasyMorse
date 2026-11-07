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
* iambickeyer.cpp is part of EasyMorse project
* **************************************************************************/

#include "iambickeyer.h"

#include <cmath>

void Paddles::set(bool pDot, bool pDash, qint64 pTimeMs)
{
    if (pDot == m_dot && pDash == m_dash)
        return;
    m_dot = pDot;
    m_dash = pDash;
    emit changed(pTimeMs);
}

IambicKeyer::IambicKeyer(QObject* parent)
    : KeyerInput(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { advanceTo(MonotonicClock::nowMs()); });
}

void IambicKeyer::setSpeed(double pWpm)
{
    // The element in progress keeps its length, the next ones follow
    m_unitMs = 1200.0 / qBound(5.0, pWpm, 60.0);
}

void IambicKeyer::setAutoTimer(bool pAutoTimer)
{
    m_autoTimer = pAutoTimer;
    if (!m_autoTimer)
        m_timer.stop();
}

void IambicKeyer::setPaddles(bool pDot, bool pDash, qint64 pTimeMs)
{
    // What was due before this change happens first
    advanceTo(pTimeMs);
    m_dot = pDot;
    m_dash = pDash;
    if (m_state == State::Idle)
    {
        // Dot first when both close together
        if (m_dot)
            startElement(Element::Dot, pTimeMs);
        else if (m_dash)
            startElement(Element::Dash, pTimeMs);
    }
    else
    {
        // Remember the opposite contact, even if released before the element ends
        if (m_dot && m_element == Element::Dash)
            m_dotMemory = true;
        if (m_dash && m_element == Element::Dot)
            m_dashMemory = true;
        if (m_dot && m_dash)
            m_squeezed = true;
    }
    armTimer();
}

void IambicKeyer::reset(qint64 pTimeMs)
{
    m_timer.stop();
    m_dot = m_dash = false;
    m_dotMemory = m_dashMemory = m_squeezed = false;
    m_state = State::Idle;
    m_element = Element::None;
    setDown(false, pTimeMs);
}

void IambicKeyer::startElement(Element pElement, qint64 pTimeMs)
{
    m_element = pElement;
    const double length = (pElement == Element::Dot ? 1.0 : 3.0) * m_unitMs;
    m_markEndMs = pTimeMs + std::llround(length);
    m_spaceEndMs = m_markEndMs + std::llround(m_unitMs);
    m_state = State::Mark;
    // The opposite contact already closed counts as touched during this element
    m_dotMemory = pElement == Element::Dash && m_dot;
    m_dashMemory = pElement == Element::Dot && m_dash;
    m_squeezed = m_dot && m_dash;
    setDown(true, pTimeMs);
}

IambicKeyer::Element IambicKeyer::nextElement() const
{
    const bool dotNext = m_element == Element::Dash;
    const bool oppositeClosed = dotNext ? m_dot : m_dash;
    const bool sameClosed = dotNext ? m_dash : m_dot;
    bool oppositeMemory = dotNext ? m_dotMemory : m_dashMemory;
    // Mode A: a squeeze released during the element ends the sending
    if (m_mode == Mode::A && m_squeezed && !m_dot && !m_dash)
        oppositeMemory = false;
    if (oppositeClosed || oppositeMemory)
        return dotNext ? Element::Dot : Element::Dash;
    if (sameClosed)
        return m_element;
    return Element::None;
}

void IambicKeyer::advanceTo(qint64 pNowMs)
{
    for (;;)
    {
        if (m_state == State::Mark && pNowMs >= m_markEndMs)
        {
            m_state = State::Space;
            // At the time it was due, not the time the timer fired
            setDown(false, m_markEndMs);
        }
        else if (m_state == State::Space && pNowMs >= m_spaceEndMs)
        {
            const Element next = nextElement();
            if (next == Element::None)
            {
                m_state = State::Idle;
                m_element = Element::None;
                break;
            }
            startElement(next, m_spaceEndMs);
        }
        else
        {
            break;
        }
    }
    armTimer();
}

void IambicKeyer::armTimer()
{
    if (!m_autoTimer)
        return;
    if (m_state == State::Idle)
    {
        m_timer.stop();
        return;
    }
    const qint64 due = m_state == State::Mark ? m_markEndMs : m_spaceEndMs;
    m_timer.start(static_cast<int>(qMax<qint64>(0, due - MonotonicClock::nowMs())));
}
