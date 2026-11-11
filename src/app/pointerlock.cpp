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
* pointerlock.cpp is part of EasyMorse project
* **************************************************************************/

#include "pointerlock.h"

#include <QCursor>
#include <QGuiApplication>

#ifdef EASYMORSE_WAYLAND_LOCK
#include <QtGui/qguiapplication_platform.h>
#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>
#include "pointer-constraints-unstable-v1-client-protocol.h"
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

PointerLock::PointerLock(QObject* parent)
    : QObject(parent)
{
}

PointerLock::~PointerLock()
{
    unlock();
#ifdef EASYMORSE_WAYLAND_LOCK
    if (m_constraints)
        zwp_pointer_constraints_v1_destroy(m_constraints);
#endif
}

bool PointerLock::lock(QWindow* pWindow, const QPoint& pCentre)
{
    unlock();
    if (!pWindow)
        return false;
    m_window = pWindow;
    m_centre = pCentre;
    const QString platform = QGuiApplication::platformName();

#ifdef EASYMORSE_WAYLAND_LOCK
    if (platform == u"wayland")
    {
        if (!lockWayland(pWindow))
            return false;
        m_method = Method::Wayland;
        m_locked = true;
        return true;
    }
#endif
#if defined(Q_OS_WIN)
    Q_UNUSED(platform)
    QCursor::setPos(pWindow->screen(), m_centre);
    POINT point;
    if (!GetCursorPos(&point))
        return false;
    const RECT rect { point.x, point.y, point.x + 1, point.y + 1 };
    if (!ClipCursor(&rect))
        return false;
    m_method = Method::Clip;
#elif defined(Q_OS_MACOS)
    Q_UNUSED(platform)
    QCursor::setPos(pWindow->screen(), m_centre);
    if (CGAssociateMouseAndMouseCursorPosition(false) != kCGErrorSuccess)
        return false;
    m_method = Method::Detach;
#else
    // X11, and XWayland which turns the moves of a hidden pointer into a lock
    if (platform != u"xcb" || !pWindow->setMouseGrabEnabled(true))
        return false;
    QCursor::setPos(pWindow->screen(), m_centre);
    m_method = Method::Grab;
#endif
    m_locked = true;
    return true;
}

void PointerLock::unlock()
{
    if (!m_locked)
        return;
    switch (m_method)
    {
    case Method::Wayland:
#ifdef EASYMORSE_WAYLAND_LOCK
        unlockWayland();
#endif
        break;
    case Method::Grab:
        if (m_window)
            m_window->setMouseGrabEnabled(false);
        break;
    case Method::Clip:
#if defined(Q_OS_WIN)
        ClipCursor(nullptr);
#endif
        break;
    case Method::Detach:
#if defined(Q_OS_MACOS)
        CGAssociateMouseAndMouseCursorPosition(true);
#endif
        break;
    case Method::None:
        break;
    }
    m_method = Method::None;
    m_locked = false;
    m_window.clear();
}

void PointerLock::pointerMoved(const QPoint& pGlobal)
{
    // Moving the pointer gives a move event too: only when away from the centre
    if (m_locked && m_method == Method::Grab && m_window && pGlobal != m_centre)
        QCursor::setPos(m_window->screen(), m_centre);
}

#ifdef EASYMORSE_WAYLAND_LOCK

void PointerLock::onGlobal(void* pData, wl_registry* pRegistry, uint32_t pName,
                           const char* pInterface, uint32_t pVersion)
{
    Q_UNUSED(pVersion)
    auto* self = static_cast<PointerLock*>(pData);
    if (qstrcmp(pInterface, zwp_pointer_constraints_v1_interface.name) == 0 && !self->m_constraints)
    {
        self->m_constraints = static_cast<zwp_pointer_constraints_v1*>(
            wl_registry_bind(pRegistry, pName, &zwp_pointer_constraints_v1_interface, 1));
    }
}

void PointerLock::onUnlocked(void* pData, zwp_locked_pointer_v1*)
{
    // A one-shot lock is over for good; the owner unlocks outside this callback
    auto* self = static_cast<PointerLock*>(pData);
    QMetaObject::invokeMethod(self, &PointerLock::lost, Qt::QueuedConnection);
}

bool PointerLock::lockWayland(QWindow* pWindow)
{
    auto* app = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
    if (!app || !native)
        return false;
    wl_pointer* pointer = app->pointer();
    auto* surface = static_cast<wl_surface*>(native->nativeResourceForWindow("surface", pWindow));
    if (!app->display() || !pointer || !surface)
        return false;

    if (!m_constraintsSearched)
    {
        // Look for the protocol on a queue of our own: Qt dispatches the main one
        m_constraintsSearched = true;
        m_display = app->display();
        wl_event_queue* queue = wl_display_create_queue(m_display);
        auto* wrapper = static_cast<wl_display*>(wl_proxy_create_wrapper(m_display));
        wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wrapper), queue);
        wl_registry* registry = wl_display_get_registry(wrapper);
        wl_proxy_wrapper_destroy(wrapper);
        static const wl_registry_listener listener { &PointerLock::onGlobal, &PointerLock::onGlobalRemove };
        wl_registry_add_listener(registry, &listener, this);
        wl_display_roundtrip_queue(m_display, queue);
        wl_registry_destroy(registry);
        // The lock events then come with Qt's own, on the main thread
        if (m_constraints)
            wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(m_constraints), nullptr);
        wl_event_queue_destroy(queue);
    }
    if (!m_constraints)
        return false;

    m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(m_constraints, surface, pointer, nullptr,
                                                              ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
    static const zwp_locked_pointer_v1_listener listener { &PointerLock::onLocked, &PointerLock::onUnlocked };
    zwp_locked_pointer_v1_add_listener(m_lockedPointer, &listener, this);
    wl_display_flush(m_display);
    return true;
}

void PointerLock::unlockWayland()
{
    if (!m_lockedPointer)
        return;
    zwp_locked_pointer_v1_destroy(m_lockedPointer);
    m_lockedPointer = nullptr;
    wl_display_flush(m_display);
}

#endif // EASYMORSE_WAYLAND_LOCK
