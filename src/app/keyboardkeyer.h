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
* The Alt key operates the straight key wherever the focus is, like in the
* previous versions. Left and right Ctrl are the dot and dash contacts of
* the paddles, as sent by the USB paddle adapters (VBand).
* **************************************************************************/

#ifndef KEYBOARDKEYER_H
#define KEYBOARDKEYER_H

#include <QObject>

#include "keyer/iambickeyer.h"

class ManualKeyer;

class KeyboardKeyer : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardKeyer(ManualKeyer* pKeyer, QObject* parent = nullptr);

    Paddles* paddles() { return &m_paddles; }

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    ManualKeyer* m_keyer;
    Paddles      m_paddles;
};

#endif // KEYBOARDKEYER_H
