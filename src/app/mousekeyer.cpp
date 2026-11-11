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
* mousekeyer.cpp is part of EasyMorse project
* **************************************************************************/

#include "mousekeyer.h"

#include <QCursor>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>

#include "keyer/keyerinput.h"
#include "settings/settings.h"
#include "tools/clogger.h"

MouseKeyer::MouseKeyer(ManualKeyer* pKeyer, Settings* pSettings, QObject* parent)
    : QObject(parent)
    , m_keyer(pKeyer)
    , m_settings(pSettings)
{
    m_releaseKey = KeyboardKeyer::parse(m_settings->releaseKey());
    connect(m_settings, &Settings::keyboardKeysChanged, this, [this] {
        m_releaseKey = KeyboardKeyer::parse(m_settings->releaseKey());
    });
    // Installed after the keyboard keys: seen first, the release key wins
    qApp->installEventFilter(this);
    // The mouse goes back to the user with the focus
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState pState) {
        if (pState != Qt::ApplicationActive)
            release();
    });
    connect(&m_lock, &PointerLock::lost, this, &MouseKeyer::release);
}

MouseKeyer::~MouseKeyer()
{
    release();
}

void MouseKeyer::capture(QQuickItem* pAnchor)
{
    QWindow* window = pAnchor ? pAnchor->window() : nullptr;
    if (!window || m_captured)
        return;
    m_window = window;
    connect(window, &QWindow::visibleChanged, this, [this](bool pVisible) {
        if (!pVisible)
            release();
    });
    const QPoint centre = pAnchor->mapToGlobal(QPointF(pAnchor->width() / 2, pAnchor->height() / 2)).toPoint();
    if (!m_lock.lock(window, centre))
    {
        CLogger::instance()->log(tr("The pointer cannot be held still here: keep it over the window"),
                                 Qt::darkYellow);
    }
    QGuiApplication::setOverrideCursor(Qt::BlankCursor);
    m_captured = true;
    emit capturedChanged();
}

void MouseKeyer::release()
{
    if (!m_captured)
        return;
    m_captured = false;
    m_lock.unlock();
    if (m_window)
        disconnect(m_window, &QWindow::visibleChanged, this, nullptr);
    m_window.clear();
    QGuiApplication::restoreOverrideCursor();
    releaseContacts();
    emit capturedChanged();
}

void MouseKeyer::releaseContacts()
{
    if (m_straightDown)
    {
        m_straightDown = false;
        m_keyer->release();
    }
    m_paddles.release();
}

void MouseKeyer::setButton(Qt::MouseButton pButton, bool pDown)
{
    // Both modes are fed: the Trainer listens to the one chosen
    if (pButton == Qt::LeftButton)
    {
        if (pDown != m_straightDown)
        {
            m_straightDown = pDown;
            pDown ? m_keyer->press() : m_keyer->release();
        }
        m_paddles.setDot(pDown);
    }
    else if (pButton == Qt::RightButton)
    {
        m_paddles.setDash(pDown);
    }
}

bool MouseKeyer::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (!m_captured || !pWatched->isWindowType())
        return QObject::eventFilter(pWatched, pEvent);

    switch (pEvent->type())
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    {
        const auto* mouse = static_cast<QMouseEvent*>(pEvent);
        setButton(mouse->button(), pEvent->type() == QEvent::MouseButtonPress);
        return true;
    }
    case QEvent::MouseMove:
        m_lock.pointerMoved(static_cast<QMouseEvent*>(pEvent)->globalPosition().toPoint());
        return true;
    // Nothing else of the mouse reaches the interface
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
    case QEvent::Enter:
    case QEvent::Leave:
        return true;
    case QEvent::KeyPress:
    {
        const auto* key = static_cast<QKeyEvent*>(pEvent);
        int code = 0;
        int side = 0;
        KeyboardKeyer::identify(key, code, side);
        if (key->isAutoRepeat() || !m_releaseKey.matches(code, side))
            break;
        release();
        return true;
    }
    default:
        break;
    }
    return QObject::eventFilter(pWatched, pEvent);
}
