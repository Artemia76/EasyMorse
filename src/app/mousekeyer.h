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
* mousekeyer.h is part of EasyMorse project
*
* Mouse wired as a Morse key: once captured, its buttons key wherever the
* pointer is and the pointer is held still (see PointerLock). The left
* button is the straight key and the dot paddle, the right button the dash
* paddle. A key chosen by the user (Escape by default) frees the mouse.
* **************************************************************************/

#ifndef MOUSEKEYER_H
#define MOUSEKEYER_H

#include <QObject>
#include <QPointer>

#include "keyboardkeyer.h"
#include "keyer/iambickeyer.h"
#include "pointerlock.h"

class ManualKeyer;
class QQuickItem;
class QWindow;
class Settings;

class MouseKeyer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool captured READ isCaptured NOTIFY capturedChanged)
    // The pointer is held still; otherwise only the clicks are captured
    Q_PROPERTY(bool locked READ isLocked NOTIFY capturedChanged)

public:
    MouseKeyer(ManualKeyer* pKeyer, Settings* pSettings, QObject* parent = nullptr);
    ~MouseKeyer() override;

    Paddles* paddles() { return &m_paddles; }

    bool    isCaptured() const { return m_captured; }
    bool    isLocked() const { return m_lock.isLocked(); }

    ///
    /// \brief capture takes the mouse, the pointer held at the centre of pAnchor
    ///
    Q_INVOKABLE void capture(QQuickItem* pAnchor);
    Q_INVOKABLE void release();

signals:
    void    capturedChanged();

protected:
    bool    eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    void    setButton(Qt::MouseButton pButton, bool pDown);
    void    releaseContacts();

    ManualKeyer*            m_keyer;
    Settings*               m_settings;
    Paddles                 m_paddles;
    PointerLock             m_lock;
    QPointer<QWindow>       m_window;
    KeyboardKeyer::Binding  m_releaseKey;
    bool                    m_captured = false;
    bool                    m_straightDown = false;
};

#endif // MOUSEKEYER_H
