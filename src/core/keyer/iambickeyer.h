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
* iambickeyer.h is part of EasyMorse project
*
* Paddles and iambic keyer. A paddle has two contacts, one for dots and
* one for dashes. The keyer turns them into a key timed by the speed:
* dots and dashes of the right length, one dot of silence between them,
* alternating while both contacts are squeezed, and remembering a contact
* touched during an element.
*
* Mode A stops after the current element when a squeeze is released;
* mode B sends one more, opposite element.
* **************************************************************************/

#ifndef IAMBICKEYER_H
#define IAMBICKEYER_H

#include <QObject>
#include <QTimer>

#include "keyer/keyerinput.h"

///
/// \brief Two contacts of a paddle: keyboard, touch screen, serial port
///
class Paddles : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dot READ dot NOTIFY changed)
    Q_PROPERTY(bool dash READ dash NOTIFY changed)

public:
    explicit Paddles(QObject* parent = nullptr) : QObject(parent) {}

    bool    dot() const { return m_dot; }
    bool    dash() const { return m_dash; }

    void    set(bool pDot, bool pDash, qint64 pTimeMs = MonotonicClock::nowMs());
    Q_INVOKABLE void setDot(bool pDown) { set(pDown, m_dash); }
    Q_INVOKABLE void setDash(bool pDown) { set(m_dot, pDown); }
    Q_INVOKABLE void release() { set(false, false); }

signals:
    void    changed(qint64 pTimeMs);

private:
    bool    m_dot = false;
    bool    m_dash = false;
};

class IambicKeyer : public KeyerInput
{
    Q_OBJECT

public:
    enum class Mode
    {
        A,
        B
    };
    Q_ENUM(Mode)

    explicit IambicKeyer(QObject* parent = nullptr);

    ///
    /// \brief speed in words per minute (PARIS): a dot lasts 1200 / speed ms
    ///
    void    setSpeed(double pWpm);
    double  speed() const { return 1200.0 / m_unitMs; }
    void    setMode(Mode pMode) { m_mode = pMode; }
    Mode    mode() const { return m_mode; }

    ///
    /// \brief when on (default) an internal timer sends the elements in
    /// real time; tests turn it off and call advanceTo() themselves
    ///
    void    setAutoTimer(bool pAutoTimer);

    ///
    /// \brief state of both contacts at pTimeMs (MonotonicClock)
    ///
    void    setPaddles(bool pDot, bool pDash, qint64 pTimeMs);
    ///
    /// \brief end the element in progress now and forget the contacts
    ///
    void    reset(qint64 pTimeMs = MonotonicClock::nowMs());
    ///
    /// \brief send what is due at pNowMs
    ///
    void    advanceTo(qint64 pNowMs);

private:
    enum class Element { None, Dot, Dash };
    enum class State { Idle, Mark, Space };

    void    startElement(Element pElement, qint64 pTimeMs);
    Element nextElement() const;
    void    armTimer();

    QTimer  m_timer;
    bool    m_autoTimer = true;
    double  m_unitMs = 60.0;    // 20 WPM
    Mode    m_mode = Mode::B;

    bool    m_dot = false;
    bool    m_dash = false;
    State   m_state = State::Idle;
    Element m_element = Element::None;
    qint64  m_markEndMs = 0;
    qint64  m_spaceEndMs = 0;
    // Contacts touched during the element, sent after it
    bool    m_dotMemory = false;
    bool    m_dashMemory = false;
    // Both contacts closed during the element
    bool    m_squeezed = false;
};

#endif // IAMBICKEYER_H
