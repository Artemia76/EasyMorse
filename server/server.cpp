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
* server.cpp is part of EasyMorse server
* **************************************************************************/

#include "server.h"

#include <QDateTime>
#include <QFile>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpServerWebSocketUpgradeResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QTcpServer>
#include <QWebSocket>

#include "protocol/signaling.h"

namespace
{
    const QString WebSocketPath = QStringLiteral("/api/v1/ws");
    // Unauthenticated sockets are closed after this delay
    constexpr int AuthTimeoutMs = 10000;
    // WebSocket messages allowed per client and second
    constexpr int MessagesPerSecond = 20;

    QHttpServerResponse json(const QJsonObject& pObject,
                             QHttpServerResponse::StatusCode pStatus = QHttpServerResponse::StatusCode::Ok)
    {
        return QHttpServerResponse(pObject, pStatus);
    }

    QHttpServerResponse error(QHttpServerResponse::StatusCode pStatus, const QString& pCode, const QString& pMessage)
    {
        return json(QJsonObject{{"error", pCode}, {"message", pMessage}}, pStatus);
    }

    QString bearerToken(const QHttpServerRequest& pRequest)
    {
        const QByteArray header = pRequest.value("authorization");
        return header.startsWith("Bearer ") ? QString::fromLatin1(header.mid(7).trimmed()) : QString();
    }
}

Server::Server(QObject* parent)
    : QObject(parent)
{
}

Server::~Server()
{
    // Sockets first: their disconnection uses the rooms
    for (auto& [socket, client] : m_clients)
        socket->disconnect(this);
    m_clients.clear();
}

bool Server::start(const Config& pConfig)
{
    m_config = pConfig;
    if (!m_accounts.open(m_config.database))
    {
        qWarning("Cannot open the database %s", qPrintable(m_config.database));
        return false;
    }
    for (const QString& room : std::as_const(m_config.permanentRooms))
    {
        const QString name = Signaling::normalizeRoom(room);
        if (!name.isEmpty())
            m_roomNames.insert(roomKey(name), name);
    }
    if (m_config.turnEnabled && !m_turn.start(m_config.turn))
    {
        qWarning("Cannot start the STUN/TURN relay on UDP port %u", m_config.turn.port);
        return false;
    }

    setupRoutes();
    m_tcp = new QTcpServer(this);
    if (!m_tcp->listen(m_config.address, m_config.port) || !m_http.bind(m_tcp))
    {
        qWarning("Cannot listen on %s:%u", qPrintable(m_config.address.toString()), m_config.port);
        return false;
    }
    return true;
}

quint16 Server::port() const
{
    return m_tcp ? m_tcp->serverPort() : 0;
}

QHostAddress Server::clientAddress(const QHttpServerRequest& pRequest) const
{
    if (m_config.trustProxy)
    {
        // The proxy appends the address it received the request from
        const QList<QByteArray> forwarded = pRequest.value("x-forwarded-for").split(',');
        const QHostAddress address(QString::fromLatin1(forwarded.last().trimmed()));
        if (!address.isNull())
            return address;
    }
    return pRequest.remoteAddress();
}

bool Server::allowAuthAttempt(const QHostAddress& pAddress)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QList<qint64>& attempts = m_authAttempts[pAddress.toString()];
    attempts.removeIf([&](qint64 time) { return now - time > m_config.authWindowSeconds; });
    if (attempts.size() >= m_config.authAttempts)
        return false;
    attempts.append(now);
    return true;
}

void Server::setupRoutes()
{
    using Method = QHttpServerRequest::Method;
    using Status = QHttpServerResponse::StatusCode;

    const auto credentials = [](const QHttpServerRequest& pRequest, QString& pCallsign, QString& pPassword) {
        const QJsonObject body = QJsonDocument::fromJson(pRequest.body()).object();
        pCallsign = body.value("callsign").toString();
        pPassword = body.value("password").toString();
        return !pCallsign.isEmpty() && !pPassword.isEmpty();
    };

    m_http.route("/api/v1/register", Method::Post, [this, credentials](const QHttpServerRequest& pRequest) {
        if (!allowAuthAttempt(clientAddress(pRequest)))
            return error(Status::TooManyRequests, "too_many_attempts", "Too many attempts, try again later");
        QString callsign, password, token;
        if (!credentials(pRequest, callsign, password))
            return error(Status::BadRequest, "bad_request", "callsign and password are required");
        switch (m_accounts.registerUser(callsign, password, token))
        {
        case Accounts::Result::Ok:
            return json({{"callsign", Signaling::normalizeCallsign(callsign)}, {"token", token}}, Status::Created);
        case Accounts::Result::InvalidCallsign:
            return error(Status::BadRequest, "invalid_callsign", "2 to 16 letters, digits, '/' or '-'");
        case Accounts::Result::WeakPassword:
            return error(Status::BadRequest, "weak_password",
                         QString("At least %1 characters").arg(Signaling::MinPasswordChars));
        case Accounts::Result::AlreadyExists:
            return error(Status::Conflict, "already_exists", "This callsign is already registered");
        default:
            return error(Status::InternalServerError, "server_error", "Server error");
        }
    });

    m_http.route("/api/v1/login", Method::Post, [this, credentials](const QHttpServerRequest& pRequest) {
        if (!allowAuthAttempt(clientAddress(pRequest)))
            return error(Status::TooManyRequests, "too_many_attempts", "Too many attempts, try again later");
        QString callsign, password, token;
        if (!credentials(pRequest, callsign, password))
            return error(Status::BadRequest, "bad_request", "callsign and password are required");
        switch (m_accounts.login(callsign, password, token))
        {
        case Accounts::Result::Ok:
            return json({{"callsign", Signaling::normalizeCallsign(callsign)}, {"token", token}});
        case Accounts::Result::WrongCredentials:
            return error(Status::Unauthorized, "wrong_credentials", "Wrong callsign or password");
        default:
            return error(Status::InternalServerError, "server_error", "Server error");
        }
    });

    m_http.route("/api/v1/logout", Method::Post, [this](const QHttpServerRequest& pRequest) {
        m_accounts.revokeToken(bearerToken(pRequest));
        return QHttpServerResponse(Status::NoContent);
    });

    m_http.route("/api/v1/rooms", Method::Get, [this]() {
        QJsonArray rooms;
        for (auto it = m_roomNames.cbegin(); it != m_roomNames.cend(); ++it)
            rooms.append(QJsonObject{{"name", it.value()}, {"members", m_rooms.value(it.key()).size()}});
        return json({{"rooms", rooms}});
    });

    m_http.route("/api/v1/status", Method::Get, [this]() {
        return json({{"version", Signaling::Version},
                     {"online", static_cast<int>(m_clients.size())},
                     {"relay", m_turn.isRunning()}});
    });

    // Web pages compiled in the resources
    const auto page = [](const QString& pFile) {
        QFile file(":/web/" + pFile);
        if (!file.open(QFile::ReadOnly))
            return QHttpServerResponse(Status::NotFound);
        const QByteArray mime = QMimeDatabase().mimeTypeForFileNameAndData(pFile, QByteArray()).name().toLatin1();
        return QHttpServerResponse(mime + "; charset=utf-8", file.readAll());
    };
    m_http.route("/", Method::Get, [page]() { return page("index.html"); });
    m_http.route("/<arg>", Method::Get, [page](const QString& pFile) {
        // Only the bundled files, no path
        if (pFile.contains(u'/') || pFile.startsWith(u'.'))
            return QHttpServerResponse(Status::NotFound);
        return page(pFile);
    });

    // WebSocket on the same port
    m_http.addWebSocketUpgradeVerifier(this, [](const QHttpServerRequest& pRequest) {
        return pRequest.url().path() == WebSocketPath ? QHttpServerWebSocketUpgradeResponse::accept()
                                                      : QHttpServerWebSocketUpgradeResponse::passToNext();
    });
    connect(&m_http, &QHttpServer::newWebSocketConnection, this, &Server::onWebSocket);
}

void Server::onWebSocket()
{
    while (m_http.hasPendingWebSocketConnections())
    {
        std::unique_ptr<QWebSocket> socket = m_http.nextPendingWebSocketConnection();
        QWebSocket* raw = socket.get();
        auto client = std::make_unique<Client>();
        client->socket = std::move(socket);
        Client* clientPtr = client.get();
        m_clients.emplace(raw, std::move(client));
        raw->setMaxAllowedIncomingMessageSize(Signaling::MaxMessageBytes);
        connect(raw, &QWebSocket::textMessageReceived, this, [this, clientPtr](const QString& pText) {
            onMessage(clientPtr, pText);
        });
        connect(raw, &QWebSocket::disconnected, this, [this, raw] {
            // Not from the socket's own signal handler
            QMetaObject::invokeMethod(this, [this, raw] { disconnectClient(raw); }, Qt::QueuedConnection);
        });
        QTimer::singleShot(AuthTimeoutMs, raw, [raw, clientPtr] {
            if (clientPtr->callsign.isEmpty())
                raw->close(QWebSocketProtocol::CloseCodePolicyViolated, "authentication timeout");
        });
    }
}

void Server::disconnectClient(QWebSocket* pSocket)
{
    const auto it = m_clients.find(pSocket);
    if (it == m_clients.end())
        return;
    leaveRoom(it->second.get());
    m_clients.erase(it);
}

void Server::onMessage(Client* pClient, const QString& pText)
{
    // Flood protection
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - pClient->windowStart >= 1000)
    {
        pClient->windowStart = now;
        pClient->messages = 0;
    }
    if (++pClient->messages > MessagesPerSecond)
    {
        pClient->socket->close(QWebSocketProtocol::CloseCodePolicyViolated, "too many messages");
        return;
    }

    const QJsonObject message = Signaling::decode(pText.toUtf8());
    const QString type = message.value("type").toString();
    if (type.isEmpty())
        return sendError(pClient, "bad_message", "Not a signaling message");
    if (pClient->callsign.isEmpty())
    {
        if (type != Signaling::Type::Auth)
            return sendError(pClient, "not_authenticated", "Authenticate first");
        return authenticate(pClient, message);
    }

    if (type == Signaling::Type::Join)
    {
        joinRoom(pClient, message.value("room").toString());
    }
    else if (type == Signaling::Type::Leave)
    {
        leaveRoom(pClient);
    }
    else if (type == Signaling::Type::Chat)
    {
        if (pClient->room.isEmpty())
            return sendError(pClient, "no_room", "Join a room first");
        const QString text = message.value("text").toString().left(Signaling::MaxChatChars).trimmed();
        if (!text.isEmpty())
            broadcast(pClient->room, Signaling::message(Signaling::Type::Chat,
                {{"room", m_roomNames.value(pClient->room)}, {"from", pClient->callsign}, {"text", text}}));
    }
    else if (type == Signaling::Type::Signal)
    {
        relaySignal(pClient, message);
    }
    else
    {
        sendError(pClient, "bad_message", "Unknown message type");
    }
}

void Server::authenticate(Client* pClient, const QJsonObject& pMessage)
{
    const QString callsign = m_accounts.callsignForToken(pMessage.value("token").toString());
    if (callsign.isEmpty())
    {
        sendError(pClient, "bad_token", "Unknown or expired token, log in again");
        pClient->socket->close(QWebSocketProtocol::CloseCodePolicyViolated, "bad token");
        return;
    }
    // One connection per callsign: the newest wins
    if (Client* previous = clientByCallsign(callsign))
    {
        sendError(previous, "replaced", "Connected from somewhere else");
        leaveRoom(previous);
        previous->callsign.clear();
        previous->socket->close(QWebSocketProtocol::CloseCodeNormal, "replaced");
    }
    pClient->callsign = callsign;

    QJsonObject ice;
    if (!m_config.publicHost.isEmpty() && m_turn.isRunning())
    {
        ice.insert("stun", QString("%1:%2").arg(m_config.publicHost).arg(m_turn.port()));
        const TurnRelay::Credentials turn = m_turn.issueCredentials();
        if (!turn.username.isEmpty())
            ice.insert("turn", QJsonObject{{"host", m_config.publicHost}, {"port", m_turn.port()},
                                           {"username", turn.username}, {"password", turn.password},
                                           {"lifetime", turn.lifetimeSeconds}});
    }
    QJsonArray rooms;
    for (const QString& name : std::as_const(m_roomNames))
        rooms.append(name);
    send(pClient, Signaling::message(Signaling::Type::Welcome,
        {{"version", Signaling::Version}, {"callsign", callsign}, {"ice", ice}, {"rooms", rooms}}));
}

void Server::joinRoom(Client* pClient, const QString& pRoom)
{
    const QString name = Signaling::normalizeRoom(pRoom);
    if (name.isEmpty())
        return sendError(pClient, "invalid_room", "Room names have 1 to 32 characters");
    const QString key = roomKey(name);
    if (pClient->room == key)
        return;
    leaveRoom(pClient);
    if (!m_roomNames.contains(key))
        m_roomNames.insert(key, name);
    broadcast(key, Signaling::message(Signaling::Type::Joined, {{"room", m_roomNames.value(key)}, {"callsign", pClient->callsign}}));
    m_rooms[key].insert(pClient);
    pClient->room = key;
    send(pClient, Signaling::message(Signaling::Type::Room,
        {{"room", m_roomNames.value(key)}, {"members", QJsonArray::fromStringList(members(key))}}));
}

void Server::leaveRoom(Client* pClient)
{
    if (pClient->room.isEmpty())
        return;
    const QString key = std::exchange(pClient->room, QString());
    QSet<Client*>& room = m_rooms[key];
    room.remove(pClient);
    if (!pClient->callsign.isEmpty())
        broadcast(key, Signaling::message(Signaling::Type::Left, {{"room", m_roomNames.value(key)}, {"callsign", pClient->callsign}}));
    if (room.isEmpty())
    {
        m_rooms.remove(key);
        // Rooms created by users disappear when empty
        bool permanent = false;
        for (const QString& name : std::as_const(m_config.permanentRooms))
            permanent = permanent || roomKey(name.simplified()) == key;
        if (!permanent)
            m_roomNames.remove(key);
    }
}

void Server::relaySignal(Client* pClient, const QJsonObject& pMessage)
{
    static const QSet<QString> kinds = {Signaling::Kind::Call, Signaling::Kind::Accept, Signaling::Kind::Decline,
                                        Signaling::Kind::Invitation, Signaling::Kind::Hangup};
    const QString kind = pMessage.value("kind").toString();
    if (!kinds.contains(kind))
        return sendError(pClient, "bad_message", "Unknown signal");
    Client* target = clientByCallsign(Signaling::normalizeCallsign(pMessage.value("to").toString()));
    // Only between members of the same room
    if (!target || target == pClient || pClient->room.isEmpty() || target->room != pClient->room)
        return sendError(pClient, "unknown_member", "This member is not in your room");
    QJsonObject relayed = Signaling::message(Signaling::Type::Signal,
                                             {{"from", pClient->callsign}, {"kind", kind}});
    if (pMessage.contains("data"))
        relayed.insert("data", pMessage.value("data"));
    send(target, relayed);
}

void Server::send(Client* pClient, const QJsonObject& pMessage)
{
    pClient->socket->sendTextMessage(QString::fromUtf8(Signaling::encode(pMessage)));
}

void Server::sendError(Client* pClient, const QString& pCode, const QString& pMessage)
{
    send(pClient, Signaling::message(Signaling::Type::Error, {{"code", pCode}, {"message", pMessage}}));
}

void Server::broadcast(const QString& pRoom, const QJsonObject& pMessage, Client* pExcept)
{
    for (Client* member : m_rooms.value(pRoom))
    {
        if (member != pExcept)
            send(member, pMessage);
    }
}

Server::Client* Server::clientByCallsign(const QString& pCallsign)
{
    if (pCallsign.isEmpty())
        return nullptr;
    for (auto& [socket, client] : m_clients)
    {
        if (client->callsign == pCallsign)
            return client.get();
    }
    return nullptr;
}

QStringList Server::members(const QString& pRoom) const
{
    QStringList list;
    for (const Client* member : m_rooms.value(pRoom))
        list.append(member->callsign);
    list.sort();
    return list;
}
