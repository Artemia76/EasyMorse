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
* pointerlock.h is part of EasyMorse project
*
* Holds the mouse pointer still over the window, as games do, so that a
* mouse wired as a Morse key keys wherever the hand moves it:
* - Wayland: pointer-constraints protocol (lock_pointer)
* - X11: mouse grab, pointer brought back to the centre after each move
* - Windows: cursor clipped to one pixel
* - macOS: mouse moves no longer move the cursor
* **************************************************************************/

#ifndef POINTERLOCK_H
#define POINTERLOCK_H

#include <QObject>
#include <cstdint>
#include <QPoint>
#include <QPointer>
#include <QWindow>

struct wl_display;
struct zwp_pointer_constraints_v1;
struct zwp_locked_pointer_v1;

class PointerLock : public QObject
{
    Q_OBJECT

public:
    explicit PointerLock(QObject* parent = nullptr);
    ~PointerLock() override;

    ///
    /// \brief lock holds the pointer at pCentre (global coordinates) over pWindow
    /// \return false when the platform cannot hold it: the pointer stays free
    ///
    bool    lock(QWindow* pWindow, const QPoint& pCentre);
    void    unlock();
    bool    isLocked() const { return m_locked; }

    ///
    /// \brief pointerMoved brings the pointer back to the centre where the
    /// platform only grabs it (X11)
    ///
    void    pointerMoved(const QPoint& pGlobal);

signals:
    ///
    /// \brief lost is emitted when the system takes the pointer back (focus
    /// given to another window...)
    ///
    void    lost();

private:
    enum class Method { None, Wayland, Grab, Clip, Detach };

#ifdef EASYMORSE_WAYLAND_LOCK
    bool    lockWayland(QWindow* pWindow);
    void    unlockWayland();
    static void onGlobal(void* pData, struct wl_registry* pRegistry, uint32_t pName,
                         const char* pInterface, uint32_t pVersion);
    static void onGlobalRemove(void*, struct wl_registry*, uint32_t) {}
    static void onLocked(void*, zwp_locked_pointer_v1*) {}
    static void onUnlocked(void* pData, zwp_locked_pointer_v1*);

    wl_display*                     m_display = nullptr;
    zwp_pointer_constraints_v1*     m_constraints = nullptr;
    zwp_locked_pointer_v1*          m_lockedPointer = nullptr;
    bool                            m_constraintsSearched = false;
#endif

    QPointer<QWindow>   m_window;
    QPoint              m_centre;
    Method              m_method = Method::None;
    bool                m_locked = false;
};

#endif // POINTERLOCK_H
