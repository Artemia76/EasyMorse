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
* protocol.cpp is part of EasyMorse project
* **************************************************************************/

#include "protocol.h"

#include <QDataStream>
#include <QIODevice>

namespace
{
    enum class Type : quint8
    {
        Hello = 1,
        Keys = 2,
        Text = 3,
        Ping = 4,
        Pong = 5,
        Bye = 6
    };

    QByteArray utf8Limited(const QString& pText, int pMaxBytes)
    {
        QByteArray bytes = pText.toUtf8();
        if (bytes.size() > pMaxBytes)
        {
            bytes.truncate(pMaxBytes);
            // Do not cut a character in the middle
            while (!bytes.isEmpty() && (static_cast<quint8>(bytes.back()) & 0xC0) == 0x80)
                bytes.chop(1);
            if (!bytes.isEmpty() && (static_cast<quint8>(bytes.back()) & 0x80))
                bytes.chop(1);
        }
        return bytes;
    }
}

QByteArray Protocol::encode(const Packet& pPacket)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << Version;
    std::visit([&out](const auto& packet) {
        using T = std::decay_t<decltype(packet)>;
        if constexpr (std::is_same_v<T, Hello>)
        {
            out << static_cast<quint8>(Type::Hello) << utf8Limited(packet.callsign, 32);
        }
        else if constexpr (std::is_same_v<T, Keys>)
        {
            const qsizetype count = qMin<qsizetype>(packet.events.size(), KeyEventsPerPacket);
            out << static_cast<quint8>(Type::Keys) << static_cast<quint8>(count);
            for (qsizetype i = packet.events.size() - count; i < packet.events.size(); ++i)
            {
                const KeyEvent& event = packet.events.at(i);
                out << event.sequence << event.timeMs << static_cast<quint8>(event.down ? 1 : 0);
            }
        }
        else if constexpr (std::is_same_v<T, Text>)
        {
            out << static_cast<quint8>(Type::Text) << packet.id << utf8Limited(packet.text, MaxTextBytes);
        }
        else if constexpr (std::is_same_v<T, Ping>)
        {
            out << static_cast<quint8>(Type::Ping) << packet.id;
        }
        else if constexpr (std::is_same_v<T, Pong>)
        {
            out << static_cast<quint8>(Type::Pong) << packet.id;
        }
        else
        {
            out << static_cast<quint8>(Type::Bye);
        }
    }, pPacket);
    return data;
}

std::optional<Protocol::Packet> Protocol::decode(const QByteArray& pData)
{
    QDataStream in(pData);
    in.setByteOrder(QDataStream::BigEndian);
    quint8 version = 0, type = 0;
    in >> version >> type;
    if (in.status() != QDataStream::Ok || version != Version)
        return std::nullopt;

    std::optional<Packet> packet;
    switch (static_cast<Type>(type))
    {
    case Type::Hello:
    {
        QByteArray callsign;
        in >> callsign;
        packet = Hello{QString::fromUtf8(callsign)};
        break;
    }
    case Type::Keys:
    {
        quint8 count = 0;
        in >> count;
        if (count > KeyEventsPerPacket)
            return std::nullopt;
        Keys keys;
        for (int i = 0; i < count; ++i)
        {
            KeyEvent event{};
            quint8 down = 0;
            in >> event.sequence >> event.timeMs >> down;
            event.down = (down != 0);
            keys.events.append(event);
        }
        packet = keys;
        break;
    }
    case Type::Text:
    {
        Text text{};
        QByteArray bytes;
        in >> text.id >> bytes;
        if (bytes.size() > MaxTextBytes)
            return std::nullopt;
        text.text = QString::fromUtf8(bytes);
        packet = text;
        break;
    }
    case Type::Ping:
    {
        Ping ping{};
        in >> ping.id;
        packet = ping;
        break;
    }
    case Type::Pong:
    {
        Pong pong{};
        in >> pong.id;
        packet = pong;
        break;
    }
    case Type::Bye:
        packet = Bye{};
        break;
    default:
        return std::nullopt;
    }
    // Truncated or padded packets are rejected
    if (in.status() != QDataStream::Ok || !in.atEnd())
        return std::nullopt;
    return packet;
}
