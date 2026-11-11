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
* signaling.cpp is part of EasyMorse project
*
* Two identical copies: src/protocol/ in EasyMorse (the application) and
* protocol/ in easymorse-server. Change both together.
* **************************************************************************/

#include "signaling.h"

#include <QJsonDocument>
#include <QRegularExpression>

QString Signaling::normalizeCallsign(const QString& pCallsign)
{
    static const QRegularExpression valid(QStringLiteral("^[A-Z0-9][A-Z0-9/-]{1,15}$"));
    const QString callsign = pCallsign.trimmed().toUpper();
    return valid.match(callsign).hasMatch() ? callsign : QString();
}

QString Signaling::normalizeRoom(const QString& pRoom)
{
    const QString room = pRoom.simplified();
    if (room.isEmpty() || room.size() > MaxRoomNameChars)
        return QString();
    for (const QChar character : room)
    {
        if (!character.isPrint())
            return QString();
    }
    return room;
}

QByteArray Signaling::encode(const QJsonObject& pMessage)
{
    return QJsonDocument(pMessage).toJson(QJsonDocument::Compact);
}

QJsonObject Signaling::decode(const QByteArray& pData)
{
    if (pData.size() > MaxMessageBytes)
        return {};
    const QJsonObject object = QJsonDocument::fromJson(pData).object();
    return object.value("type").isString() ? object : QJsonObject();
}

QJsonObject Signaling::message(const QString& pType, QJsonObject pFields)
{
    pFields.insert("type", pType);
    return pFields;
}
