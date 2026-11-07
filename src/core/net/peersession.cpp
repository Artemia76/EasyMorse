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
* peersession.cpp is part of EasyMorse project
* **************************************************************************/

#include "peersession.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

#include "keyer/monotonicclock.h"
#include "net/securechannel.h"
#include "tools/clogger.h"

namespace
{
    // "EM1:" then base64url of the compressed JSON
    const QString InvitationPrefix = QStringLiteral("EM1:");
    // The last key event is sent again after these delays, in case the
    // packet carrying it is lost and no other key change follows
    constexpr int KeyRepeatsMs[] = {40, 150};
}

PeerSession::PeerSession(QObject* parent)
    : QObject(parent)
{
    connect(&m_link, &PeerLink::stateChanged, this, &PeerSession::onLinkState);
    connect(&m_link, &PeerLink::datagramReceived, this, &PeerSession::onDatagram);
    connect(&m_link, &PeerLink::gatheringDone, this, [this] {
        if (m_state != State::Preparing || !m_channel)
            return;
        const QJsonObject invitation{
            {"v", 1},
            {"call", m_callsign},
            {"key", QString::fromLatin1(m_channel->publicKey().toBase64())},
            {"ice", m_link.localDescription()},
        };
        const QByteArray json = QJsonDocument(invitation).toJson(QJsonDocument::Compact);
        m_invitation = InvitationPrefix + QString::fromLatin1(
            qCompress(json, 9).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        emit invitationChanged();
        setState(State::Inviting);
        // The peer's invitation may have been given before ours was ready
        if (!m_pendingInvitation.isEmpty())
            join(std::exchange(m_pendingInvitation, QString()));
    });
    connect(&m_jitter, &JitterBuffer::keyChanged, &m_remoteKeyer, &RemoteKeyer::apply);
    m_pingTimer.setInterval(PingIntervalMs);
    connect(&m_pingTimer, &QTimer::timeout, this, &PeerSession::ping);
    // The UDP path may open before the peer has pasted our invitation (its
    // ICE agent answers probes already): hello is repeated until it answers
    m_helloTimer.setInterval(1000);
    connect(&m_helloTimer, &QTimer::timeout, this, [this] { sendPacket(Protocol::Hello{m_callsign}); });
}

PeerSession::~PeerSession()
{
    stop();
}

QString PeerSession::fingerprint() const
{
    return m_channel ? m_channel->fingerprint() : QString();
}

void PeerSession::start()
{
    stop();
    m_channel = std::make_unique<SecureChannel>();
    m_invitation.clear();
    m_peerCallsign.clear();
    m_pendingInvitation.clear();
    m_keyEvents.clear();
    m_nextKeySequence = 0;
    m_pingsSent.clear();
    m_roundTripMs = -1;
    m_jitter.reset();
    m_startMs = MonotonicClock::nowMs();
    emit invitationChanged();
    emit peerChanged();
    emit roundTripChanged();
    setState(State::Preparing);
    if (!m_link.start())
        setState(State::Failed);
}

QString PeerSession::join(const QString& pInvitation)
{
    const QString code = pInvitation.trimmed();
    if (!code.startsWith(InvitationPrefix))
        return tr("This is not an EasyMorse invitation");
    const QByteArray compressed = QByteArray::fromBase64(code.mid(InvitationPrefix.size()).toLatin1(),
                                                         QByteArray::Base64UrlEncoding);
    const QJsonObject invitation = QJsonDocument::fromJson(qUncompress(compressed)).object();
    const QByteArray peerKey = QByteArray::fromBase64(invitation.value("key").toString().toLatin1());
    const QString description = invitation.value("ice").toString();
    if (invitation.value("v").toInt() != 1 || peerKey.size() != SecureChannel::KeySize || description.isEmpty())
        return tr("This invitation is damaged or comes from another version");

    if (m_state == State::Idle || m_state == State::Disconnected || m_state == State::Failed)
        start();
    if (m_state == State::Preparing)
    {
        // Our own invitation is not ready yet: join when it is
        m_pendingInvitation = code;
        return QString();
    }
    if (m_state != State::Inviting)
        return tr("A session is already in progress");
    if (peerKey == m_channel->publicKey())
        return tr("This is your own invitation: give it to your correspondent and paste theirs");
    if (!m_channel->setPeerPublicKey(peerKey))
        return tr("This invitation is damaged or comes from another version");
    if (!m_link.setRemoteDescription(description))
        return tr("This invitation is damaged or comes from another version");
    m_peerCallsign = invitation.value("call").toString().left(32);
    emit peerChanged();
    setState(State::Connecting);
    return QString();
}

void PeerSession::stop()
{
    if (m_state == State::Connected)
        sendPacket(Protocol::Bye{});
    endSession(State::Idle);
    m_link.close();
    m_channel.reset();
    m_invitation.clear();
    emit invitationChanged();
}

void PeerSession::endSession(State pState)
{
    m_pingTimer.stop();
    m_helloTimer.stop();
    m_jitter.reset();
    // Never leave the remote key down
    m_remoteKeyer.apply(false, MonotonicClock::nowMs());
    // The invitation of an ended session is useless: a new one is needed
    if (!m_invitation.isEmpty())
    {
        m_invitation.clear();
        emit invitationChanged();
    }
    setState(pState);
}

bool PeerSession::sendText(const QString& pText)
{
    if (m_state != State::Connected || pText.isEmpty())
        return false;
    sendPacket(Protocol::Text{m_nextTextId++, pText});
    return true;
}

void PeerSession::sendKey(bool pDown, qint64 pTimeMs)
{
    if (m_state != State::Connected)
        return;
    m_keyEvents.append({m_nextKeySequence++, static_cast<quint32>(pTimeMs - m_startMs), pDown});
    while (m_keyEvents.size() > Protocol::KeyEventsPerPacket)
        m_keyEvents.removeFirst();
    sendKeys();
    const quint32 sequence = m_keyEvents.last().sequence;
    for (int delay : KeyRepeatsMs)
    {
        QTimer::singleShot(delay, this, [this, sequence] {
            // Repeat only if no newer event was sent meanwhile
            if (!m_keyEvents.isEmpty() && m_keyEvents.last().sequence == sequence)
                sendKeys();
        });
    }
}

void PeerSession::sendKeys()
{
    sendPacket(Protocol::Keys{m_keyEvents});
}

void PeerSession::sendPacket(const Protocol::Packet& pPacket)
{
    if (!m_channel || !m_channel->isReady())
        return;
    m_link.send(m_channel->seal(Protocol::encode(pPacket)));
}

void PeerSession::ping()
{
    const qint64 now = MonotonicClock::nowMs();
    if (now - m_lastReceivedMs > TimeoutMs)
    {
        CLogger::instance()->log(tr("Connection with %1 lost").arg(m_peerCallsign), Qt::darkMagenta);
        endSession(State::Disconnected);
        m_link.close();
        return;
    }
    // Pings also keep the NAT mappings open
    const quint32 id = m_nextPingId++;
    m_pingsSent.insert(id, now);
    // Forget unanswered pings
    for (auto it = m_pingsSent.begin(); it != m_pingsSent.end();)
        it = (now - it.value() > TimeoutMs) ? m_pingsSent.erase(it) : std::next(it);
    sendPacket(Protocol::Ping{id});
}

void PeerSession::onLinkState(PeerLink::State pState)
{
    switch (pState)
    {
    case PeerLink::State::Connected:
        if (m_state == State::Connecting && !m_helloTimer.isActive())
        {
            sendPacket(Protocol::Hello{m_callsign});
            m_helloTimer.start();
        }
        break;
    case PeerLink::State::Failed:
        if (m_state == State::Connected)
            endSession(State::Disconnected);
        else if (m_state != State::Idle)
            endSession(State::Failed);
        break;
    default:
        break;
    }
}

void PeerSession::onDatagram(const QByteArray& pDatagram)
{
    if (!m_channel)
        return;
    const std::optional<QByteArray> plain = m_channel->open(pDatagram);
    if (!plain)
        return;
    const std::optional<Protocol::Packet> packet = Protocol::decode(*plain);
    if (!packet)
        return;
    const qint64 now = MonotonicClock::nowMs();
    m_lastReceivedMs = now;
    // An authentic packet: the peer has our key, the session is open
    if (m_state == State::Connecting)
        peerConfirmed();

    if (const auto* keys = std::get_if<Protocol::Keys>(&*packet))
    {
        m_jitter.push(keys->events, now);
    }
    else if (const auto* text = std::get_if<Protocol::Text>(&*packet))
    {
        emit textReceived(text->text);
    }
    else if (const auto* ping = std::get_if<Protocol::Ping>(&*packet))
    {
        sendPacket(Protocol::Pong{ping->id});
    }
    else if (const auto* pong = std::get_if<Protocol::Pong>(&*packet))
    {
        const auto sent = m_pingsSent.find(pong->id);
        if (sent != m_pingsSent.end())
        {
            m_roundTripMs = static_cast<int>(now - sent.value());
            m_pingsSent.erase(sent);
            emit roundTripChanged();
        }
    }
    else if (const auto* hello = std::get_if<Protocol::Hello>(&*packet))
    {
        // The callsign announced on the encrypted link wins over the invitation
        if (hello->callsign != m_peerCallsign)
        {
            m_peerCallsign = hello->callsign.left(32);
            emit peerChanged();
        }
    }
    else if (std::holds_alternative<Protocol::Bye>(*packet))
    {
        CLogger::instance()->log(tr("%1 left the session").arg(m_peerCallsign), Qt::darkMagenta);
        endSession(State::Disconnected);
        m_link.close();
    }
}

void PeerSession::peerConfirmed()
{
    m_helloTimer.stop();
    // Our hello may have been sent before the peer could read it
    sendPacket(Protocol::Hello{m_callsign});
    setState(State::Connected);
    m_pingTimer.start();
    ping();
    CLogger::instance()->log(tr("Connected with %1 (%2)").arg(m_peerCallsign, m_link.pathType()), Qt::darkGreen);
}

void PeerSession::setState(State pState)
{
    if (pState == m_state)
        return;
    m_state = pState;
    emit stateChanged();
}
