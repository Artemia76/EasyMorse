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
* morsecodec.h is part of EasyMorse project
*
* International Morse code table: character <-> pattern of '.' and '-'
* **************************************************************************/

#ifndef MORSECODEC_H
#define MORSECODEC_H

#include <QChar>
#include <QList>
#include <QPair>
#include <QString>

namespace MorseCodec
{
    ///
    /// \brief encode a character, case insensitive
    /// \return its pattern (e.g. ".-" for A), empty if it has no Morse code
    ///
    QString encode(QChar pChar);

    ///
    /// \brief decode a pattern of '.' and '-'
    /// \return the character, empty if the pattern is unknown
    ///
    QString decode(const QString& pPattern);

    ///
    /// \brief table lists every supported character with its pattern
    ///
    const QList<QPair<QChar, QString>>& table();
}

#endif // MORSECODEC_H
