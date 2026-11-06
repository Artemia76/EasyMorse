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
* protocol.h is part of EasyMorse project
*
* Packets exchanged between two EasyMorse over the peer to peer link (they
* are encrypted by SecureChannel). Morse is sent as key events, not audio.
* **************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>
#include <variant>

namespace Protocol
{
    constexpr quint8 Version = 1;
    // Each key packet repeats the last events, so a lost packet is recovered
    // by the next one
    constexpr int KeyEventsPerPacket = 8;
    constexpr int MaxTextBytes = 500;

    struct KeyEvent
    {
        quint32 sequence;   // increases by one for every key change
        quint32 timeMs;     // sender time since the session start
        bool    down;

        bool operator==(const KeyEvent&) const = default;
    };

    struct Hello
    {
        QString callsign;
        bool operator==(const Hello&) const = default;
    };

    struct Keys
    {
        QList<KeyEvent> events;
        bool operator==(const Keys&) const = default;
    };

    struct Text
    {
        quint32 id;
        QString text;
        bool operator==(const Text&) const = default;
    };

    struct Ping
    {
        quint32 id;
        bool operator==(const Ping&) const = default;
    };

    struct Pong
    {
        quint32 id;
        bool operator==(const Pong&) const = default;
    };

    struct Bye
    {
        bool operator==(const Bye&) const = default;
    };

    using Packet = std::variant<Hello, Keys, Text, Ping, Pong, Bye>;

    QByteArray              encode(const Packet& pPacket);
    ///
    /// \return the packet, or nothing if the data is not a valid packet of this version
    ///
    std::optional<Packet>   decode(const QByteArray& pData);
}

#endif // PROTOCOL_H
