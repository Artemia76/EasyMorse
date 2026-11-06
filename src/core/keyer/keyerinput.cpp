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
* keyerinput.cpp is part of EasyMorse project
* **************************************************************************/

#include "keyerinput.h"

KeyerInput::KeyerInput(QObject* parent)
    : QObject(parent)
{
}

void KeyerInput::setDown(bool pDown, qint64 pTimeMs)
{
    // Repeated presses (keyboard auto repeat) are not key changes
    if (pDown == m_down)
        return;
    m_down = pDown;
    emit keyChanged(pDown, pTimeMs);
}

ManualKeyer::ManualKeyer(QObject* parent)
    : KeyerInput(parent)
{
}
