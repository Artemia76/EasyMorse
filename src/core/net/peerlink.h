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
* peerlink.h is part of EasyMorse project
*
* UDP link to another EasyMorse through NAT routers, based on libjuice (ICE):
* each side gathers its candidate addresses (local, public seen by a STUN
* server, relay), the descriptions are exchanged, then both sides probe the
* candidate pairs at the same time until one goes through (hole punching).
* **************************************************************************/

#ifndef PEERLINK_H
#define PEERLINK_H

#include <QByteArray>
#include <QObject>
#include <QString>

struct juice_agent;

class PeerLink : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Idle,
        Gathering,      // collecting local candidates
        Ready,          // local description available, waiting for the peer's
        Connecting,     // probing candidate pairs
        Connected,
        Failed,
        Closed
    };
    Q_ENUM(State)

    struct Server
    {
        QString host;
        quint16 port = 0;
        QString username;   // TURN only
        QString password;   // TURN only
    };

    explicit PeerLink(QObject* parent = nullptr);
    ~PeerLink() override;

    void    setStunServer(const Server& pServer) { m_stun = pServer; }
    void    setTurnServer(const Server& pServer) { m_turn = pServer; }

    ///
    /// \brief start gathering candidates; localDescription() is ready on gatheringDone()
    ///
    bool    start();
    void    close();

    State   state() const { return m_state; }
    QString localDescription() const { return m_localDescription; }
    ///
    /// \brief give the peer description (with its candidates) and start connecting
    ///
    bool    setRemoteDescription(const QString& pDescription);
    bool    send(const QByteArray& pDatagram);

    ///
    /// \brief kind of path in use: "host" (same network), "srflx" (through
    /// NAT), "relay" (TURN), empty when not connected
    ///
    QString pathType() const { return m_pathType; }

signals:
    void    stateChanged(PeerLink::State pState);
    void    gatheringDone();
    void    datagramReceived(const QByteArray& pDatagram);

private:
    struct Callbacks;

    void    setState(State pState);

    juice_agent*    m_agent = nullptr;
    Server          m_stun;
    Server          m_turn;
    State           m_state = State::Idle;
    QString         m_localDescription;
    QString         m_pathType;
    bool            m_remoteSet = false;
};

#endif // PEERLINK_H
