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

#include "net/peersession.h"
#include "protocol/signaling.h"
#include "tools/clogger.h"

Lobby::Lobby(PeerSession* pSession, QObject* parent)
    : QObject(parent)
    , m_session(pSession)
{
    connect(&m_client, &SignalingClient::signalReceived, this, &Lobby::onSignal);
    connect(&m_client, &SignalingClient::memberLeft, this, [this](const QString& pCallsign) {
        if (pCallsign == m_peer && (m_callState == CallState::Calling || m_callState == CallState::Ringing))
        {
            emit callFailed(m_peer, "left");
            setCall(CallState::Idle);
        }
    });
    connect(&m_client, &SignalingClient::stateChanged, this, [this] {
        // A call not answered yet needs the server; an open session does not
        if (m_client.state() != SignalingClient::State::Online
            && (m_callState == CallState::Calling || m_callState == CallState::Ringing))
            setCall(CallState::Idle);
    });

    // The invitation is sent through the server as soon as it is ready
    connect(m_session, &PeerSession::invitationChanged, this, [this] {
        if (m_callState == CallState::InCall && !m_invitationSent && !m_session->invitation().isEmpty())
        {
            m_invitationSent = true;
            m_client.sendSignal(m_peer, Signaling::Kind::Invitation, m_session->invitation());
        }
    });
    connect(m_session, &PeerSession::stateChanged, this, [this] {
        const PeerSession::State state = m_session->state();
        if (m_callState == CallState::InCall
            && (state == PeerSession::State::Disconnected || state == PeerSession::State::Failed
                || state == PeerSession::State::Idle))
            setCall(CallState::Idle);
    });

    m_callTimer.setSingleShot(true);
    m_callTimer.setInterval(CallTimeoutMs);
    connect(&m_callTimer, &QTimer::timeout, this, [this] {
        if (m_callState == CallState::Calling)
        {
            m_client.sendSignal(m_peer, Signaling::Kind::Hangup);
            emit callFailed(m_peer, "timeout");
            setCall(CallState::Idle);
        }
    });
}

bool Lobby::call(const QString& pCallsign)
{
    if (m_client.state() != SignalingClient::State::Online || m_callState != CallState::Idle
        || pCallsign == m_client.callsign() || !m_client.members().contains(pCallsign))
        return false;
    const PeerSession::State session = m_session->state();
    if (session == PeerSession::State::Connected || session == PeerSession::State::Connecting)
        return false;
    m_client.sendSignal(pCallsign, Signaling::Kind::Call);
    setCall(CallState::Calling, pCallsign);
    m_callTimer.start();
    return true;
}

void Lobby::accept()
{
    if (m_callState != CallState::Ringing)
        return;
    m_client.sendSignal(m_peer, Signaling::Kind::Accept);
    setCall(CallState::InCall, m_peer);
    startSession();
}

void Lobby::decline()
{
    if (m_callState != CallState::Ringing)
        return;
    m_client.sendSignal(m_peer, Signaling::Kind::Decline);
    setCall(CallState::Idle);
}

void Lobby::hangup()
{
    if (m_callState == CallState::Idle)
        return;
    m_client.sendSignal(m_peer, Signaling::Kind::Hangup);
    if (m_callState == CallState::InCall)
        m_session->stop();
    setCall(CallState::Idle);
}

void Lobby::onSignal(const QString& pFrom, const QString& pKind, const QString& pData)
{
    if (pKind == Signaling::Kind::Call)
    {
        const PeerSession::State session = m_session->state();
        const bool busy = m_callState != CallState::Idle || session == PeerSession::State::Connected
                          || session == PeerSession::State::Connecting;
        if (busy)
        {
            m_client.sendSignal(pFrom, Signaling::Kind::Decline, "busy");
            return;
        }
        setCall(CallState::Ringing, pFrom);
        emit incomingCall(pFrom);
        return;
    }
    // Everything else only concerns the current call
    if (pFrom != m_peer)
        return;
    if (pKind == Signaling::Kind::Accept && m_callState == CallState::Calling)
    {
        m_callTimer.stop();
        setCall(CallState::InCall, m_peer);
        startSession();
    }
    else if (pKind == Signaling::Kind::Decline && m_callState == CallState::Calling)
    {
        emit callFailed(m_peer, pData == "busy" ? QStringLiteral("busy") : QStringLiteral("declined"));
        setCall(CallState::Idle);
    }
    else if (pKind == Signaling::Kind::Invitation && m_callState == CallState::InCall)
    {
        const QString error = m_session->join(pData);
        if (!error.isEmpty())
        {
            CLogger::instance()->log(error, Qt::red);
            hangup();
        }
    }
    else if (pKind == Signaling::Kind::Hangup)
    {
        if (m_callState == CallState::InCall)
            m_session->stop();
        if (m_callState == CallState::Ringing)
            emit callFailed(m_peer, "timeout");
        setCall(CallState::Idle);
    }
}

void Lobby::startSession()
{
    // Relay and public address servers given by the match-making server
    const QJsonObject ice = m_client.ice();
    const QString stun = ice.value("stun").toString();
    const qsizetype colon = stun.lastIndexOf(u':');
    if (colon > 0)
        m_session->setStunServer({stun.left(colon), static_cast<quint16>(stun.mid(colon + 1).toUInt()), {}, {}});
    const QJsonObject turn = ice.value("turn").toObject();
    if (!turn.isEmpty())
        m_session->setTurnServer({turn.value("host").toString(), static_cast<quint16>(turn.value("port").toInt()),
                                  turn.value("username").toString(), turn.value("password").toString()});
    m_invitationSent = false;
    m_session->start();
}

void Lobby::setCall(CallState pState, const QString& pPeer)
{
    if (pState != CallState::Calling)
        m_callTimer.stop();
    if (pState == m_callState && pPeer == m_peer)
        return;
    m_callState = pState;
    m_peer = pPeer;
    emit callStateChanged();
}
