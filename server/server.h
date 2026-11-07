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
* server.h is part of EasyMorse server
*
* Match-making server: REST API for accounts and rooms, WebSocket on the
* same port for presence, room chat and the signals that let two members
* open a peer to peer session (see protocol/signaling.h), static web pages,
* and the STUN/TURN relay. Plain HTTP: TLS is done by a reverse proxy.
* **************************************************************************/

#ifndef SERVER_H
#define SERVER_H

#include <QHash>
#include <QHostAddress>
#include <QHttpServer>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include <map>
#include <memory>

#include "accounts.h"
#include "turnrelay.h"

class QWebSocket;
class QTcpServer;

class Server : public QObject
{
    Q_OBJECT

public:
    struct Config
    {
        QHostAddress    address = QHostAddress::LocalHost;
        quint16         port = 8080;            // 0: any free port (tests)
        QString         database = QStringLiteral("easymorse.db");
        // Behind a reverse proxy: client addresses come from X-Forwarded-For
        bool            trustProxy = false;
        QStringList     permanentRooms = {QStringLiteral("Beginners"), QStringLiteral("QSO")};
        // Host name clients use for STUN/TURN (empty: STUN/TURN not offered)
        QString         publicHost;
        bool            turnEnabled = false;
        TurnRelay::Config turn;
        // Login and register attempts allowed per address and window
        int             authAttempts = 10;
        int             authWindowSeconds = 600;
    };

    explicit Server(QObject* parent = nullptr);
    ~Server() override;

    bool    start(const Config& pConfig);
    quint16 port() const;

private:
    struct Client
    {
        std::unique_ptr<QWebSocket> socket;
        QString                     callsign;   // empty until authenticated
        QString                     room;
        int                         messages = 0;   // in the current second
        qint64                      windowStart = 0;
    };

    void    setupRoutes();
    void    onWebSocket();
    void    onMessage(Client* pClient, const QString& pText);
    void    authenticate(Client* pClient, const QJsonObject& pMessage);
    void    joinRoom(Client* pClient, const QString& pRoom);
    void    leaveRoom(Client* pClient);
    void    relaySignal(Client* pClient, const QJsonObject& pMessage);
    void    send(Client* pClient, const QJsonObject& pMessage);
    void    sendError(Client* pClient, const QString& pCode, const QString& pMessage);
    void    broadcast(const QString& pRoom, const QJsonObject& pMessage, Client* pExcept = nullptr);
    void    disconnectClient(QWebSocket* pSocket);
    Client* clientByCallsign(const QString& pCallsign);
    QStringList members(const QString& pRoom) const;
    QString roomKey(const QString& pRoom) const { return pRoom.toCaseFolded(); }
    bool    allowAuthAttempt(const QHostAddress& pAddress);
    QHostAddress clientAddress(const class QHttpServerRequest& pRequest) const;

    Config                                      m_config;
    QHttpServer                                 m_http;
    QTcpServer*                                 m_tcp = nullptr;
    Accounts                                    m_accounts;
    TurnRelay                                   m_turn;
    std::map<QWebSocket*, std::unique_ptr<Client>> m_clients;
    // Room key -> display name and members
    QMap<QString, QString>                      m_roomNames;
    QHash<QString, QSet<Client*>>               m_rooms;
    // Address -> attempt times (s) in the current window
    QHash<QString, QList<qint64>>               m_authAttempts;
};

#endif // SERVER_H
