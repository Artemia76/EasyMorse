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
#include <QKeySequence>
#include <QWindow>

#include "keyer/keyerinput.h"
#include "settings/settings.h"

#ifdef Q_OS_MACOS
constexpr int ControlKey = Qt::Key_Meta;    // Qt::Key_Control is Command on macOS
#else
constexpr int ControlKey = Qt::Key_Control;
#endif

KeyboardKeyer::KeyboardKeyer(ManualKeyer* pKeyer, Settings* pSettings, QObject* parent)
    : QObject(parent)
    , m_keyer(pKeyer)
    , m_settings(pSettings)
{
    loadBindings();
    connect(m_settings, &Settings::keyboardKeysChanged, this, [this] {
        releaseAll();
        loadBindings();
    });
    qApp->installEventFilter(this);
    // A key held while the window loses the focus would never be released
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState pState) {
        if (pState != Qt::ApplicationActive)
            releaseAll();
    });
}

KeyboardKeyer::Binding KeyboardKeyer::parse(const QString& pKey)
{
    const QStringList parts = pKey.split(u':');
    Binding binding;
    binding.key = parts.value(0).toInt();
    binding.side = qBound(0, parts.value(1).toInt(), 2);
    return binding;
}

void KeyboardKeyer::loadBindings()
{
    m_straight = parse(m_settings->straightKey());
    m_dot = parse(m_settings->dotKey());
    m_dash = parse(m_settings->dashKey());
}

void KeyboardKeyer::releaseAll()
{
    m_keyer->release();
    m_paddles.release();
}

bool KeyboardKeyer::isModifier(int pKey)
{
    return pKey == Qt::Key_Control || pKey == Qt::Key_Alt || pKey == Qt::Key_Shift || pKey == Qt::Key_Meta;
}

void KeyboardKeyer::identify(const QKeyEvent* pEvent, int& pKey, int& pSide)
{
    pKey = pEvent->key();
    pSide = 0;
    // AltGr is the right Alt key of many layouts
    if (pKey == Qt::Key_AltGr)
    {
        pKey = Qt::Key_Alt;
        pSide = 2;
        return;
    }
    if (!isModifier(pKey))
        return;
#if defined(Q_OS_MACOS)
    // Virtual key codes of the right Command, Shift, Option and Control keys
    const quint32 code = pEvent->nativeVirtualKey();
    pSide = (code == 0x36 || code == 0x3C || code == 0x3D || code == 0x3E) ? 2 : 1;
#elif defined(Q_OS_WIN)
    // Right Ctrl and Alt are extended keys; right Shift has its own scan code
    const quint32 code = pEvent->nativeScanCode();
    pSide = ((code & 0x100) || code == 0x36) ? 2 : 1;
#else
    // X11 / Wayland (evdev) keycodes of Control_R, Alt_R, Shift_R, Super_R
    const quint32 code = pEvent->nativeScanCode();
    pSide = (code == 105 || code == 108 || code == 62 || code == 134) ? 2 : 1;
#endif
}

void KeyboardKeyer::capture(const QString& pRole)
{
    releaseAll();
    m_capturing = pRole;
    emit capturingChanged();
}

void KeyboardKeyer::cancelCapture()
{
    if (m_capturing.isEmpty())
        return;
    m_capturing.clear();
    emit capturingChanged();
}

QString KeyboardKeyer::keyName(const QString& pKey) const
{
    const Binding binding = parse(pKey);
    if (binding.key == 0)
        return tr("None");
    QString name;
    if (binding.key == ControlKey)
        name = tr("Ctrl");
    else if (binding.key == Qt::Key_Alt)
#ifdef Q_OS_MACOS
        name = tr("Option");
#else
        name = tr("Alt");
#endif
    else if (binding.key == Qt::Key_Shift)
        name = tr("Shift");
    else if (binding.key == Qt::Key_Meta || binding.key == Qt::Key_Control)
#ifdef Q_OS_MACOS
        name = tr("Command");
#else
        name = tr("System");
#endif
    else if (binding.key == Qt::Key_Space)
        name = tr("Space");
    else
        name = QKeySequence(binding.key).toString(QKeySequence::NativeText);
    if (binding.side == 1)
        return tr("Left %1").arg(name);
    if (binding.side == 2)
        return tr("Right %1").arg(name);
    return name;
}

void KeyboardKeyer::useRightHandedKeys()
{
    // Left hand on the keys, side by side
    m_settings->setDotKey(format(ControlKey, 1));
    m_settings->setDashKey(format(Qt::Key_Alt, 1));
}

void KeyboardKeyer::useLeftHandedKeys()
{
    // The same, mirrored on the right of the space bar
    m_settings->setDotKey(format(ControlKey, 2));
    m_settings->setDashKey(format(Qt::Key_Alt, 2));
}

void KeyboardKeyer::useAdapterKeys()
{
    // As sent by USB paddle adapters (VBand)
    m_settings->setDotKey(format(ControlKey, 1));
    m_settings->setDashKey(format(ControlKey, 2));
}

bool KeyboardKeyer::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    const QEvent::Type type = pEvent->type();
    // Shortcut overrides would let a printable key reach a shortcut first
    if ((type != QEvent::KeyPress && type != QEvent::KeyRelease) || !pWatched->isWindowType())
        return QObject::eventFilter(pWatched, pEvent);
    auto* keyEvent = static_cast<QKeyEvent*>(pEvent);
    int key = 0;
    int side = 0;
    identify(keyEvent, key, side);
    if (key == 0 || key == Qt::Key_unknown)
        return QObject::eventFilter(pWatched, pEvent);

    // Choosing a key in the options
    if (!m_capturing.isEmpty())
    {
        if (type == QEvent::KeyPress && !keyEvent->isAutoRepeat())
        {
            if (key != Qt::Key_Escape)
            {
                const QString value = format(key, side);
                if (m_capturing == u"straight")
                    m_settings->setStraightKey(value);
                else if (m_capturing == u"dot")
                    m_settings->setDotKey(value);
                else if (m_capturing == u"dash")
                    m_settings->setDashKey(value);
            }
            cancelCapture();
        }
        return true;
    }

    const bool straight = m_straight.matches(key, side);
    const bool dot = m_dot.matches(key, side);
    const bool dash = m_dash.matches(key, side);
    if (!straight && !dot && !dash)
        return QObject::eventFilter(pWatched, pEvent);

    // A printable key, or AltGr, types in text fields instead of keying
    if (!isModifier(key) || keyEvent->key() == Qt::Key_AltGr)
    {
        const QObject* focus = QGuiApplication::focusObject();
        if (focus && (focus->inherits("QQuickTextInput") || focus->inherits("QQuickTextEdit")))
            return QObject::eventFilter(pWatched, pEvent);
    }
    if (!keyEvent->isAutoRepeat())
    {
        const bool down = type == QEvent::KeyPress;
        // Both modes are fed: the Trainer listens to the one chosen
        if (straight)
            down ? m_keyer->press() : m_keyer->release();
        if (dot)
            m_paddles.setDot(down);
        if (dash)
            m_paddles.setDash(down);
    }
    // Ctrl is left to the shortcuts; Alt must not open menus or move the
    // focus, a printable key must not type or trigger anything
    return key != ControlKey && key != Qt::Key_Shift;
}
