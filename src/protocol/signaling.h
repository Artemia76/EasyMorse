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
* signaling.h is part of EasyMorse project
*
* Messages between EasyMorse and the match-making server, shared by both.
* JSON text frames on a WebSocket, after authentication with a token
* obtained from the REST API (/api/v1/register or /api/v1/login).
*
* client -> server                      server -> client
*   auth   {token}                        welcome {callsign, ice}
*   join   {room}                         room    {room, members[]}
*   leave  {}                             joined  {room, callsign}
*   chat   {text}                         left    {room, callsign}
*   signal {to, kind, data}               chat    {room, from, text}
*                                         signal  {from, kind, data}
*                                         error   {code, message}
*
* Signal kinds, relayed between two members of the same room:
*   call (A asks B for a session), accept, decline, invitation (the
*   PeerSession invitation code), hangup.
* **************************************************************************/

#ifndef SIGNALING_H
#define SIGNALING_H

#include <QJsonObject>
#include <QString>

namespace Signaling
{
    constexpr int Version = 1;
    // Larger frames are refused by the server
    constexpr int MaxMessageBytes = 8192;
    constexpr int MaxChatChars = 500;
    constexpr int MaxRoomNameChars = 32;
    constexpr int MaxCallsignChars = 16;
    constexpr int MinPasswordChars = 8;

    namespace Type
    {
        inline const QString Auth = QStringLiteral("auth");
        inline const QString Welcome = QStringLiteral("welcome");
        inline const QString Join = QStringLiteral("join");
        inline const QString Leave = QStringLiteral("leave");
        inline const QString Room = QStringLiteral("room");
        inline const QString Joined = QStringLiteral("joined");
        inline const QString Left = QStringLiteral("left");
        inline const QString Chat = QStringLiteral("chat");
        inline const QString Signal = QStringLiteral("signal");
        inline const QString Error = QStringLiteral("error");
    }

    namespace Kind
    {
        inline const QString Call = QStringLiteral("call");
        inline const QString Accept = QStringLiteral("accept");
        inline const QString Decline = QStringLiteral("decline");
        inline const QString Invitation = QStringLiteral("invitation");
        inline const QString Hangup = QStringLiteral("hangup");
    }

    ///
    /// \brief callsign rules: 2 to 16 letters, digits, '/' or '-', stored upper case
    /// \return the normalized callsign, empty if invalid
    ///
    QString normalizeCallsign(const QString& pCallsign);
    ///
    /// \return the room name trimmed, empty if invalid (1 to 32 printable characters)
    ///
    QString normalizeRoom(const QString& pRoom);

    QByteArray  encode(const QJsonObject& pMessage);
    ///
    /// \return the message, empty if it is not a JSON object with a "type"
    ///
    QJsonObject decode(const QByteArray& pData);

    QJsonObject message(const QString& pType, QJsonObject pFields = {});
}

#endif // SIGNALING_H
