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
* Room of the match-making server: a peer to peer session is opened by
* itself with every other member of the room (full mesh), with the
* invitations exchanged through the server. Lost links are retried while
* the member stays in the room.
* **************************************************************************/

#ifndef LOBBY_H
#define LOBBY_H

#include <QList>
#include <QObject>
#include <QTimer>

#include <map>

#include "net/peerlink.h"
#include "net/peersession.h"
#include "net/signalingclient.h"

class Lobby : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SignalingClient* client READ client CONSTANT)
    Q_PROPERTY(QList<PeerSession*> peers READ peers NOTIFY peersChanged)
    Q_PROPERTY(int connectedCount READ connectedCount NOTIFY connectedCountChanged)

public:
    // A session still not open after this long is started again
    static constexpr int StallTimeoutMs = 20000;
    static constexpr int FirstRetryMs = 3000;
    static constexpr int MaxRetryMs = 60000;

    explicit Lobby(QObject* parent = nullptr);
    ~Lobby() override;

    SignalingClient* client() { return &m_client; }

    ///
    /// \brief sessions with the other members of the room, by callsign
    ///
    QList<PeerSession*> peers() const;
    ///
    /// \return the session with this member, null if none
    ///
    Q_INVOKABLE PeerSession* peer(const QString& pCallsign) const;
    int     connectedCount() const;

    ///
    /// \brief STUN server used when the match-making server offers none
    ///
    void    setFallbackStun(const PeerLink::Server& pServer) { m_fallbackStun = pServer; }
    void    setJitterDelay(int pMs);

public slots:
    ///
    /// \brief send a change of the local key to every connected member
    ///
    void    sendKey(bool pDown, qint64 pTimeMs);

signals:
    void    peersChanged();
    void    peerAdded(PeerSession* pSession);
    ///
    /// \brief the session is deleted right after
    ///
    void    peerRemoved(PeerSession* pSession);
    void    connectedCountChanged();

private:
    struct Peer
    {
        PeerSession*    session = nullptr;
        QString         sentInvitation;
        qint64          stateSince = 0;
        qint64          retryAt = 0;    // 0: no retry planned
        int             retryDelayMs = 0;
    };

    void    sync();
    Peer&   addPeer(const QString& pCallsign);
    void    removePeer(const QString& pCallsign);
    void    clearPeers();
    void    configure(PeerSession* pSession);
    void    restart(Peer& pPeer);
    void    onSignal(const QString& pFrom, const QString& pKind, const QString& pData);
    void    tick();
    ///
    /// \brief one side only retries, so both do not restart at once
    ///
    bool    leads(const QString& pCallsign) const { return m_client.callsign() < pCallsign; }

    SignalingClient             m_client;
    std::map<QString, Peer>     m_peers;
    QTimer                      m_tick;
    PeerLink::Server            m_fallbackStun;
    int                         m_jitterDelayMs = -1;
    int                         m_connected = 0;
};

#endif // LOBBY_H
