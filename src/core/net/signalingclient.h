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
* signalingclient.h is part of EasyMorse project
*
* Connection to the match-making server: account (REST), then a WebSocket
* for presence, rooms, room chat and the signals used to open peer to peer
* sessions. Reconnects by itself while logged in.
* **************************************************************************/

#ifndef SIGNALINGCLIENT_H
#define SIGNALINGCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class QNetworkAccessManager;
class QWebSocket;

class SignalingClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString callsign READ callsign NOTIFY stateChanged)
    Q_PROPERTY(QString room READ room NOTIFY roomChanged)
    Q_PROPERTY(QStringList members READ members NOTIFY roomChanged)
    Q_PROPERTY(QVariantList rooms READ rooms NOTIFY roomsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)

public:
    enum class State
    {
        Offline,
        LoggingIn,      // REST request in progress
        Connecting,     // WebSocket opening or authenticating
        Online
    };
    Q_ENUM(State)

    explicit SignalingClient(QObject* parent = nullptr);
    ~SignalingClient() override;

    ///
    /// \brief server address, e.g. https://morse.example.org (http for tests)
    ///
    void    setServerUrl(const QUrl& pUrl) { m_serverUrl = pUrl; }
    QUrl    serverUrl() const { return m_serverUrl; }

    State       state() const { return m_state; }
    QString     callsign() const { return m_callsign; }
    QString     token() const { return m_token; }
    QString     room() const { return m_room; }
    QStringList members() const { return m_members; }
    QVariantList rooms() const { return m_rooms; }
    QString     lastError() const { return m_lastError; }
    ///
    /// \brief STUN/TURN offered by the server in its welcome
    ///
    QJsonObject ice() const { return m_ice; }

    Q_INVOKABLE void login(const QString& pCallsign, const QString& pPassword);
    Q_INVOKABLE void registerAccount(const QString& pCallsign, const QString& pPassword);
    ///
    /// \brief connect with a token saved from a previous login
    ///
    Q_INVOKABLE void resume(const QString& pToken);
    Q_INVOKABLE void logout();

    Q_INVOKABLE void refreshRooms();
    Q_INVOKABLE void joinRoom(const QString& pRoom);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void sendChat(const QString& pText);
    void    sendSignal(const QString& pTo, const QString& pKind, const QString& pData = QString());

signals:
    void    stateChanged();
    void    tokenChanged(const QString& pToken);
    void    roomChanged();
    void    roomsChanged();
    void    chatReceived(const QString& pFrom, const QString& pText);
    void    signalReceived(const QString& pFrom, const QString& pKind, const QString& pData);
    void    memberLeft(const QString& pCallsign);
    void    errorOccurred(const QString& pMessage);

private:
    void    authenticate(const QString& pPath, const QString& pCallsign, const QString& pPassword);
    void    openSocket();
    void    onMessage(const QString& pText);
    void    send(const QJsonObject& pMessage);
    void    setState(State pState);
    void    fail(const QString& pMessage);
    QUrl    apiUrl(const QString& pPath) const;
    ///
    /// \brief message for an error code of the server, in the user language
    ///
    QString errorText(const QString& pCode, const QString& pServerMessage) const;

    QNetworkAccessManager*  m_network;
    QWebSocket*             m_socket;
    QTimer                  m_reconnectTimer;
    int                     m_reconnectDelayMs = 0;
    QUrl                    m_serverUrl;
    State                   m_state = State::Offline;
    QString                 m_token;
    QString                 m_callsign;
    QString                 m_room;
    QString                 m_wantedRoom;   // rejoined after a reconnection
    QStringList             m_members;
    QVariantList            m_rooms;
    QString                 m_lastError;
    QJsonObject             m_ice;
};

#endif // SIGNALINGCLIENT_H
