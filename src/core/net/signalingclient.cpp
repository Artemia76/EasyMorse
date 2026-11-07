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
* signalingclient.cpp is part of EasyMorse project
* **************************************************************************/

#include "signalingclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebSocket>

#include "protocol/signaling.h"

namespace
{
    constexpr int FirstReconnectDelayMs = 2000;
    constexpr int MaxReconnectDelayMs = 60000;
}

SignalingClient::SignalingClient(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    connect(m_socket, &QWebSocket::connected, this, [this] {
        send(Signaling::message(Signaling::Type::Auth, {{"token", m_token}}));
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, &SignalingClient::onMessage);
    connect(m_socket, &QWebSocket::disconnected, this, [this] {
        const bool wasOnline = m_state == State::Online || m_state == State::Connecting;
        m_room.clear();
        m_members.clear();
        emit roomChanged();
        if (m_token.isEmpty() || !wasOnline)
        {
            setState(State::Offline);
            return;
        }
        // Lost: try again later with the same token
        setState(State::Connecting);
        m_reconnectDelayMs = qBound(FirstReconnectDelayMs, m_reconnectDelayMs * 2, MaxReconnectDelayMs);
        m_reconnectTimer.start(m_reconnectDelayMs);
    });
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &SignalingClient::openSocket);
}

SignalingClient::~SignalingClient()
{
    m_socket->disconnect(this);
}

QUrl SignalingClient::apiUrl(const QString& pPath) const
{
    QUrl url = m_serverUrl;
    url.setPath(pPath);
    return url;
}

void SignalingClient::login(const QString& pCallsign, const QString& pPassword)
{
    authenticate("/api/v1/login", pCallsign, pPassword);
}

void SignalingClient::registerAccount(const QString& pCallsign, const QString& pPassword)
{
    authenticate("/api/v1/register", pCallsign, pPassword);
}

void SignalingClient::authenticate(const QString& pPath, const QString& pCallsign, const QString& pPassword)
{
    if (!m_serverUrl.isValid() || m_serverUrl.host().isEmpty())
        return fail(tr("Set the server address first"));
    setState(State::LoggingIn);
    QNetworkRequest request(apiUrl(pPath));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray body = QJsonDocument(QJsonObject{{"callsign", pCallsign}, {"password", pPassword}}).toJson();
    QNetworkReply* reply = m_network->post(request, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
        const QString token = answer.value("token").toString();
        if (token.isEmpty())
        {
            const QString code = answer.value("error").toString();
            return fail(code.isEmpty() ? tr("Server unreachable: %1").arg(reply->errorString())
                                       : errorText(code, answer.value("message").toString()));
        }
        m_token = token;
        emit tokenChanged(m_token);
        m_reconnectDelayMs = 0;
        openSocket();
    });
}

void SignalingClient::resume(const QString& pToken)
{
    if (pToken.isEmpty() || !m_serverUrl.isValid())
        return;
    m_token = pToken;
    m_reconnectDelayMs = 0;
    openSocket();
}

void SignalingClient::openSocket()
{
    QUrl url = apiUrl("/api/v1/ws");
    url.setScheme(m_serverUrl.scheme() == "https" ? "wss" : "ws");
    setState(State::Connecting);
    m_socket->abort();
    m_socket->open(url);
}

void SignalingClient::logout()
{
    m_reconnectTimer.stop();
    if (!m_token.isEmpty())
    {
        QNetworkRequest request(apiUrl("/api/v1/logout"));
        request.setRawHeader("Authorization", "Bearer " + m_token.toLatin1());
        QNetworkReply* reply = m_network->post(request, QByteArray());
        connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }
    m_token.clear();
    m_wantedRoom.clear();
    emit tokenChanged(QString());
    m_socket->close();
    setState(State::Offline);
}

void SignalingClient::refreshRooms()
{
    QNetworkReply* reply = m_network->get(QNetworkRequest(apiUrl("/api/v1/rooms")));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_rooms = QJsonDocument::fromJson(reply->readAll()).object().value("rooms").toArray().toVariantList();
        emit roomsChanged();
    });
}

void SignalingClient::joinRoom(const QString& pRoom)
{
    m_wantedRoom = pRoom;
    send(Signaling::message(Signaling::Type::Join, {{"room", pRoom}}));
}

void SignalingClient::leaveRoom()
{
    m_wantedRoom.clear();
    send(Signaling::message(Signaling::Type::Leave));
    m_room.clear();
    m_members.clear();
    emit roomChanged();
}

void SignalingClient::sendChat(const QString& pText)
{
    send(Signaling::message(Signaling::Type::Chat, {{"text", pText}}));
}

void SignalingClient::sendSignal(const QString& pTo, const QString& pKind, const QString& pData)
{
    QJsonObject message{{"to", pTo}, {"kind", pKind}};
    if (!pData.isEmpty())
        message.insert("data", pData);
    send(Signaling::message(Signaling::Type::Signal, message));
}

void SignalingClient::send(const QJsonObject& pMessage)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->sendTextMessage(QString::fromUtf8(Signaling::encode(pMessage)));
}

void SignalingClient::onMessage(const QString& pText)
{
    const QJsonObject message = Signaling::decode(pText.toUtf8());
    const QString type = message.value("type").toString();
    if (type == Signaling::Type::Welcome)
    {
        m_callsign = message.value("callsign").toString();
        m_ice = message.value("ice").toObject();
        m_reconnectDelayMs = 0;
        setState(State::Online);
        if (!m_wantedRoom.isEmpty())
            joinRoom(m_wantedRoom);
    }
    else if (type == Signaling::Type::Room)
    {
        m_room = message.value("room").toString();
        m_members = message.value("members").toVariant().toStringList();
        emit roomChanged();
    }
    else if (type == Signaling::Type::Joined)
    {
        const QString member = message.value("callsign").toString();
        if (!m_members.contains(member))
        {
            m_members.append(member);
            m_members.sort();
            emit roomChanged();
        }
    }
    else if (type == Signaling::Type::Left)
    {
        const QString member = message.value("callsign").toString();
        if (m_members.removeAll(member) > 0)
            emit roomChanged();
        emit memberLeft(member);
    }
    else if (type == Signaling::Type::Chat)
    {
        emit chatReceived(message.value("from").toString(), message.value("text").toString());
    }
    else if (type == Signaling::Type::Signal)
    {
        emit signalReceived(message.value("from").toString(), message.value("kind").toString(),
                            message.value("data").toString());
    }
    else if (type == Signaling::Type::Error)
    {
        const QString code = message.value("code").toString();
        if (code == "bad_token" || code == "replaced")
        {
            // Do not reconnect with a token the server refuses
            m_token.clear();
            if (code == "bad_token")
                emit tokenChanged(QString());
        }
        m_lastError = errorText(code, message.value("message").toString());
        emit errorOccurred(m_lastError);
    }
}

QString SignalingClient::errorText(const QString& pCode, const QString& pServerMessage) const
{
    if (pCode == "invalid_callsign")
        return tr("Invalid callsign: 2 to 16 letters, digits, '/' or '-'");
    if (pCode == "weak_password")
        return tr("The password needs at least 8 characters");
    if (pCode == "already_exists")
        return tr("This callsign is already registered");
    if (pCode == "wrong_credentials")
        return tr("Wrong callsign or password");
    if (pCode == "too_many_attempts")
        return tr("Too many attempts, try again later");
    if (pCode == "bad_token")
        return tr("Session expired, log in again");
    if (pCode == "replaced")
        return tr("Logged in from somewhere else");
    if (pCode == "unknown_member")
        return tr("This operator is no longer in the room");
    return pServerMessage.isEmpty() ? tr("Server error") : pServerMessage;
}

void SignalingClient::setState(State pState)
{
    if (pState == m_state)
        return;
    m_state = pState;
    emit stateChanged();
}

void SignalingClient::fail(const QString& pMessage)
{
    m_lastError = pMessage;
    setState(State::Offline);
    emit errorOccurred(pMessage);
}
