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
* lobby.h is part of EasyMorse project
*
* Calls between members of a room of the match-making server: call, ring,
* accept or decline, then the peer to peer session is opened with the
* invitations exchanged through the server (no copy and paste).
* **************************************************************************/

#ifndef LOBBY_H
#define LOBBY_H

#include <QObject>
#include <QTimer>

#include "net/signalingclient.h"

class PeerSession;

class Lobby : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CallState callState READ callState NOTIFY callStateChanged)
    Q_PROPERTY(QString callPeer READ callPeer NOTIFY callStateChanged)
    Q_PROPERTY(SignalingClient* client READ client CONSTANT)

public:
    enum class CallState
    {
        Idle,
        Calling,    // waiting for the member called to answer
        Ringing,    // a member is calling us
        InCall      // session opening or open
    };
    Q_ENUM(CallState)

    static constexpr int CallTimeoutMs = 30000;

    explicit Lobby(PeerSession* pSession, QObject* parent = nullptr);

    SignalingClient* client() { return &m_client; }
    CallState   callState() const { return m_callState; }
    QString     callPeer() const { return m_peer; }

    Q_INVOKABLE bool call(const QString& pCallsign);
    Q_INVOKABLE void accept();
    Q_INVOKABLE void decline();
    Q_INVOKABLE void hangup();

signals:
    void    callStateChanged();
    void    incomingCall(const QString& pFrom);
    ///
    /// \param pReason "declined", "busy", "timeout" or "left"
    ///
    void    callFailed(const QString& pPeer, const QString& pReason);

private:
    void    onSignal(const QString& pFrom, const QString& pKind, const QString& pData);
    void    startSession();
    void    setCall(CallState pState, const QString& pPeer = QString());

    PeerSession*    m_session;
    SignalingClient m_client;
    QTimer          m_callTimer;
    CallState       m_callState = CallState::Idle;
    QString         m_peer;
    bool            m_invitationSent = false;
};

#endif // LOBBY_H
