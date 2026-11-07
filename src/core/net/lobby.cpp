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
* lobby.cpp is part of EasyMorse project
* **************************************************************************/

#include "lobby.h"

#include <QJsonObject>

#include "keyer/monotonicclock.h"
#include "net/peersession.h"
#include "protocol/signaling.h"
#include "tools/clogger.h"

Lobby::Lobby(QObject* parent)
    : QObject(parent)
{
    connect(&m_client, &SignalingClient::signalReceived, this, &Lobby::onSignal);
    connect(&m_client, &SignalingClient::roomChanged, this, &Lobby::sync);
    connect(&m_client, &SignalingClient::stateChanged, this, &Lobby::sync);
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &Lobby::tick);
}

Lobby::~Lobby()
{
    disconnect(&m_client, nullptr, this, nullptr);
    clearPeers();
}

QList<PeerSession*> Lobby::peers() const
{
    QList<PeerSession*> list;
    for (const auto& [callsign, peer] : m_peers)
        list.append(peer.session);
    return list;
}

PeerSession* Lobby::peer(const QString& pCallsign) const
{
    const auto it = m_peers.find(pCallsign);
    return it == m_peers.end() ? nullptr : it->second.session;
}

int Lobby::connectedCount() const
{
    return m_connected;
}

void Lobby::setJitterDelay(int pMs)
{
    m_jitterDelayMs = pMs;
    for (auto& [callsign, peer] : m_peers)
        peer.session->setJitterDelay(pMs);
}

void Lobby::sendKey(bool pDown, qint64 pTimeMs)
{
    // Only connected sessions send
    for (auto& [callsign, peer] : m_peers)
        peer.session->sendKey(pDown, pTimeMs);
}

///
/// \brief Lobby::sync opens a session with each member of the room and
/// closes those of the members gone
///
void Lobby::sync()
{
    const SignalingClient::State state = m_client.state();
    if (state == SignalingClient::State::Offline)
        return clearPeers();
    // While the server is reconnecting the open sessions go on: they do not use it
    if (state != SignalingClient::State::Online)
        return;
    QStringList wanted;
    if (!m_client.room().isEmpty())
        wanted = m_client.members();
    wanted.removeAll(m_client.callsign());

    QStringList gone;
    for (const auto& [callsign, peer] : m_peers)
    {
        if (!wanted.contains(callsign))
            gone.append(callsign);
    }
    for (const QString& callsign : std::as_const(gone))
        removePeer(callsign);
    for (const QString& callsign : std::as_const(wanted))
    {
        if (m_peers.find(callsign) == m_peers.end())
            restart(addPeer(callsign));
    }
    if (m_peers.empty())
        m_tick.stop();
    else if (!m_tick.isActive())
        m_tick.start();
}

Lobby::Peer& Lobby::addPeer(const QString& pCallsign)
{
    auto* session = new PeerSession(this);
    Peer& peer = m_peers[pCallsign];
    peer.session = session;
    peer.stateSince = MonotonicClock::nowMs();
    configure(session);

    // Our invitation goes to the member through the server once ready
    connect(session, &PeerSession::invitationChanged, this, [this, pCallsign, session] {
        const auto it = m_peers.find(pCallsign);
        const QString invitation = session->invitation();
        if (it == m_peers.end() || invitation.isEmpty() || invitation == it->second.sentInvitation)
            return;
        it->second.sentInvitation = invitation;
        m_client.sendSignal(pCallsign, Signaling::Kind::Invitation, invitation);
    });
    connect(session, &PeerSession::stateChanged, this, [this, pCallsign, session] {
        const auto it = m_peers.find(pCallsign);
        if (it == m_peers.end())
            return;
        it->second.stateSince = MonotonicClock::nowMs();
        if (session->state() == PeerSession::State::Connected)
        {
            it->second.retryDelayMs = 0;
            it->second.retryAt = 0;
        }
        int connected = 0;
        for (const auto& [callsign, peer] : m_peers)
            connected += peer.session->state() == PeerSession::State::Connected ? 1 : 0;
        if (connected != m_connected)
        {
            m_connected = connected;
            emit connectedCountChanged();
        }
    });
    emit peerAdded(session);
    emit peersChanged();
    return peer;
}

void Lobby::removePeer(const QString& pCallsign)
{
    const auto it = m_peers.find(pCallsign);
    if (it == m_peers.end())
        return;
    PeerSession* session = it->second.session;
    m_peers.erase(it);
    session->disconnect(this);
    session->stop();
    emit peerRemoved(session);
    session->deleteLater();
    emit peersChanged();
    int connected = 0;
    for (const auto& [callsign, peer] : m_peers)
        connected += peer.session->state() == PeerSession::State::Connected ? 1 : 0;
    if (connected != m_connected)
    {
        m_connected = connected;
        emit connectedCountChanged();
    }
}

void Lobby::clearPeers()
{
    m_tick.stop();
    while (!m_peers.empty())
        removePeer(m_peers.begin()->first);
}

void Lobby::configure(PeerSession* pSession)
{
    pSession->setCallsign(m_client.callsign());
    if (m_jitterDelayMs >= 0)
        pSession->setJitterDelay(m_jitterDelayMs);
    // Relay and public address servers given by the match-making server
    const QJsonObject ice = m_client.ice();
    const QString stun = ice.value("stun").toString();
    const qsizetype colon = stun.lastIndexOf(u':');
    if (colon > 0)
        pSession->setStunServer({stun.left(colon), static_cast<quint16>(stun.mid(colon + 1).toUInt()), {}, {}});
    else
        pSession->setStunServer(m_fallbackStun);
    const QJsonObject turn = ice.value("turn").toObject();
    if (!turn.isEmpty())
        pSession->setTurnServer({turn.value("host").toString(), static_cast<quint16>(turn.value("port").toInt()),
                                 turn.value("username").toString(), turn.value("password").toString()});
    else
        pSession->setTurnServer({});
}

void Lobby::restart(Peer& pPeer)
{
    pPeer.retryAt = 0;
    pPeer.stateSince = MonotonicClock::nowMs();
    configure(pPeer.session);
    pPeer.session->start();
}

void Lobby::onSignal(const QString& pFrom, const QString& pKind, const QString& pData)
{
    if (pKind != Signaling::Kind::Invitation || pFrom == m_client.callsign())
        return;
    auto it = m_peers.find(pFrom);
    if (it == m_peers.end())
    {
        // Its arrival in the room may not be known yet
        if (m_client.state() != SignalingClient::State::Online || m_client.room().isEmpty())
            return;
        addPeer(pFrom);
        it = m_peers.find(pFrom);
        if (!m_tick.isActive())
            m_tick.start();
    }
    PeerSession* session = it->second.session;
    // Each invitation comes from a new start: an open session is replaced
    const PeerSession::State state = session->state();
    if (state == PeerSession::State::Connecting || state == PeerSession::State::Connected)
        session->stop();
    configure(session);
    const QString error = session->join(pData);
    if (!error.isEmpty())
        CLogger::instance()->log(QStringLiteral("%1: %2").arg(pFrom, error), Qt::red);
}

///
/// \brief Lobby::tick starts again the sessions lost or stuck, with a growing delay
///
void Lobby::tick()
{
    const qint64 now = MonotonicClock::nowMs();
    for (auto& [callsign, peer] : m_peers)
    {
        if (!leads(callsign))
            continue;
        const PeerSession::State state = peer.session->state();
        const bool lost = state == PeerSession::State::Failed || state == PeerSession::State::Disconnected
                          || state == PeerSession::State::Idle;
        const bool stuck = !lost && state != PeerSession::State::Connected
                           && now - peer.stateSince > StallTimeoutMs;
        if (!lost && !stuck)
            continue;
        if (peer.retryAt == 0)
        {
            peer.retryDelayMs = qBound(FirstRetryMs, peer.retryDelayMs * 2, MaxRetryMs);
            peer.retryAt = now + (stuck ? 0 : peer.retryDelayMs);
        }
        if (now >= peer.retryAt)
            restart(peer);
    }
}
