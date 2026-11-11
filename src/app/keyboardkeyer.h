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
* keyboardkeyer.h is part of EasyMorse project
*
* Keys of the computer keyboard used as a Morse key wherever the focus is:
* one key for the straight key, two for the dot and dash paddles. Each one
* is chosen by the user (left and right Ctrl, Alt, Shift... are told
* apart); a printable key does not key while a text field has the focus.
* **************************************************************************/

#ifndef KEYBOARDKEYER_H
#define KEYBOARDKEYER_H

#include <QObject>
#include <QString>

#include "keyer/iambickeyer.h"

class ManualKeyer;
class QKeyEvent;
class Settings;

class KeyboardKeyer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString capturing READ capturing NOTIFY capturingChanged)

public:
    KeyboardKeyer(ManualKeyer* pKeyer, Settings* pSettings, QObject* parent = nullptr);

    Paddles* paddles() { return &m_paddles; }

    ///
    /// \brief the next key pressed becomes the key of pRole ("straight", "dot",
    /// "dash" or "release", which ends the capture of the mouse); Escape
    /// cancels, except for "release" where it is a valid choice
    ///
    Q_INVOKABLE void capture(const QString& pRole);
    Q_INVOKABLE void cancelCapture();
    QString capturing() const { return m_capturing; }

    ///
    /// \brief name of a key setting ("<key>:<side>") in the user language, e.g. "Left Ctrl"
    ///
    Q_INVOKABLE QString keyName(const QString& pKey) const;

    // Ready-made choices of paddle keys
    Q_INVOKABLE void useRightHandedKeys();
    Q_INVOKABLE void useLeftHandedKeys();
    Q_INVOKABLE void useAdapterKeys();

    ///
    /// \brief key setting ("<key>:<side>") read back
    ///
    struct Binding
    {
        int key = 0;
        int side = 0;   // 0 either, 1 left, 2 right
        bool matches(int pKey, int pSide) const { return key == pKey && (side == 0 || side == pSide); }
    };

    static Binding parse(const QString& pKey);
    static QString format(int pKey, int pSide) { return QString("%1:%2").arg(pKey).arg(pSide); }
    ///
    /// \brief Qt key and side (1 left, 2 right, 0 for keys that have one only)
    ///
    static void identify(const QKeyEvent* pEvent, int& pKey, int& pSide);
    static bool isModifier(int pKey);

signals:
    void    capturingChanged();

protected:
    bool    eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    void    loadBindings();
    void    releaseAll();

    ManualKeyer*    m_keyer;
    Settings*       m_settings;
    Paddles         m_paddles;
    Binding         m_straight;
    Binding         m_dot;
    Binding         m_dash;
    QString         m_capturing;
};

#endif // KEYBOARDKEYER_H
