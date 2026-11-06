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
* keyerinput.h is part of EasyMorse project
*
* Base class of every way to operate the Morse key: keyboard, touch screen,
* serial port, remote station...
* **************************************************************************/

#ifndef KEYERINPUT_H
#define KEYERINPUT_H

#include <QObject>

#include "keyer/monotonicclock.h"

class KeyerInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool down READ isDown NOTIFY keyChanged)

public:
    explicit KeyerInput(QObject* parent = nullptr);

    bool    isDown() const { return m_down; }

signals:
    ///
    /// \brief keyChanged is emitted on every real change of the key
    /// \param pDown key pressed
    /// \param pTimeMs time of the change, see MonotonicClock
    ///
    void    keyChanged(bool pDown, qint64 pTimeMs);

protected:
    void    setDown(bool pDown, qint64 pTimeMs = MonotonicClock::nowMs());

private:
    bool    m_down = false;
};

///
/// \brief Key operated by the user interface: keyboard key, button, touch area
///
class ManualKeyer : public KeyerInput
{
    Q_OBJECT

public:
    explicit ManualKeyer(QObject* parent = nullptr);

    Q_INVOKABLE void press() { setDown(true); }
    Q_INVOKABLE void release() { setDown(false); }
};

#endif // KEYERINPUT_H
