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
* keyboardkeyer.cpp is part of EasyMorse project
* **************************************************************************/

#include "keyboardkeyer.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>

#include "keyer/keyerinput.h"

namespace
{
    ///
    /// \return 0 if this is not a Ctrl key, 1 for the left one, 2 for the right one
    ///
    int ctrlSide(const QKeyEvent* pEvent)
    {
#if defined(Q_OS_MACOS)
        // Qt::Key_Control is Command on macOS, the Control key is Key_Meta
        if (pEvent->key() != Qt::Key_Meta)
            return 0;
        return pEvent->nativeVirtualKey() == 0x3E ? 2 : 1;     // kVK_RightControl
#elif defined(Q_OS_WIN)
        if (pEvent->key() != Qt::Key_Control)
            return 0;
        // Right Ctrl is an extended key
        return (pEvent->nativeScanCode() & 0x100) ? 2 : 1;
#else
        if (pEvent->key() != Qt::Key_Control)
            return 0;
        return pEvent->nativeScanCode() == 105 ? 2 : 1;         // X11 / Wayland keycode of Control_R
#endif
    }
}

KeyboardKeyer::KeyboardKeyer(ManualKeyer* pKeyer, QObject* parent)
    : QObject(parent)
    , m_keyer(pKeyer)
{
    qApp->installEventFilter(this);
    // A key held while the window loses the focus would never be released
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState pState) {
        if (pState != Qt::ApplicationActive)
        {
            m_keyer->release();
            m_paddles.release();
        }
    });
}

bool KeyboardKeyer::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    const QEvent::Type type = pEvent->type();
    if ((type == QEvent::KeyPress || type == QEvent::KeyRelease) && pWatched->isWindowType())
    {
        auto* keyEvent = static_cast<QKeyEvent*>(pEvent);
        if (keyEvent->key() == Qt::Key_Alt)
        {
            if (!keyEvent->isAutoRepeat())
            {
                if (type == QEvent::KeyPress)
                    m_keyer->press();
                else
                    m_keyer->release();
            }
            // Alt must not open menus or move the focus while keying
            return true;
        }
        // Paddles: not consumed, Ctrl shortcuts keep working
        if (const int side = ctrlSide(keyEvent); side > 0 && !keyEvent->isAutoRepeat())
        {
            const bool down = type == QEvent::KeyPress;
            if (side == 1)
                m_paddles.setDot(down);
            else
                m_paddles.setDash(down);
        }
    }
    return QObject::eventFilter(pWatched, pEvent);
}
