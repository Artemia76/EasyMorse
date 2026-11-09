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
* peerlink.cpp is part of EasyMorse project
* **************************************************************************/

#include "peerlink.h"

#include <QMetaObject>

#include <juice/juice.h>

#ifdef Q_OS_DARWIN
#include <csignal>
#endif

#include "tools/clogger.h"

///
/// \brief libjuice calls these from its own thread: everything is forwarded
/// to the Qt thread of the link
///
struct PeerLink::Callbacks
{
    static void stateChanged(juice_agent_t* pAgent, juice_state_t pState, void* pUser)
    {
        auto* link = static_cast<PeerLink*>(pUser);
        QMetaObject::invokeMethod(link, [link, pAgent, pState] {
            // Ignore an agent closed or replaced meanwhile
            if (link->m_agent != pAgent)
                return;
            switch (pState)
            {
            case JUICE_STATE_CONNECTING:
                // libjuice reports it as soon as gathering ends: it only
                // means something once the peer description is known
                if (link->m_remoteSet)
                    link->setState(State::Connecting);
                break;
            case JUICE_STATE_CONNECTED:
            case JUICE_STATE_COMPLETED:
            {
                char local[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
                char remote[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
                if (link->m_agent && juice_get_selected_candidates(link->m_agent, local, sizeof(local),
                                                                   remote, sizeof(remote)) == 0)
                {
                    // "a=candidate:... typ host|srflx|prflx|relay ..."
                    const QStringList localParts = QString::fromLatin1(local).split(u' ');
                    const QStringList remoteParts = QString::fromLatin1(remote).split(u' ');
                    const auto typeOf = [](const QStringList& pParts) {
                        const qsizetype index = pParts.indexOf("typ");
                        return index >= 0 && index + 1 < pParts.size() ? pParts.at(index + 1) : QString();
                    };
                    const QString localType = typeOf(localParts);
                    const QString remoteType = typeOf(remoteParts);
                    link->m_pathType = (localType == "relay" || remoteType == "relay") ? QString("relay")
                                     : (localType == "host" && remoteType == "host") ? QString("host")
                                     : QString("srflx");
                }
                link->setState(State::Connected);
                break;
            }
            case JUICE_STATE_FAILED:
                link->setState(State::Failed);
                break;
            default:
                break;
            }
        }, Qt::QueuedConnection);
    }

    static void gatheringDone(juice_agent_t* pAgent, void* pUser)
    {
        auto* link = static_cast<PeerLink*>(pUser);
        char buffer[JUICE_MAX_SDP_STRING_LEN];
        QString description;
        if (juice_get_local_description(pAgent, buffer, sizeof(buffer)) == 0)
            description = QString::fromLatin1(buffer);
        QMetaObject::invokeMethod(link, [link, pAgent, description] {
            if (link->m_agent != pAgent)
                return;
            link->m_localDescription = description;
            if (link->m_state == State::Gathering)
                link->setState(State::Ready);
            emit link->gatheringDone();
        }, Qt::QueuedConnection);
    }

    static void received(juice_agent_t* pAgent, const char* pData, size_t pSize, void* pUser)
    {
        auto* link = static_cast<PeerLink*>(pUser);
        const QByteArray datagram(pData, static_cast<qsizetype>(pSize));
        QMetaObject::invokeMethod(link, [link, pAgent, datagram] {
            if (link->m_agent != pAgent)
                return;
            emit link->datagramReceived(datagram);
        }, Qt::QueuedConnection);
    }
};

PeerLink::PeerLink(QObject* parent)
    : QObject(parent)
{
    // libjuice messages go to the application log (from libjuice threads)
    static const bool logRouted = [] {
#ifdef Q_OS_DARWIN
        // On Apple systems a UDP sendto() can fail with EPIPE, and libjuice
        // cannot ask not to get the signal that would kill the application
        std::signal(SIGPIPE, SIG_IGN);
#endif
        juice_set_log_level(JUICE_LOG_LEVEL_WARN);
        juice_set_log_handler([](juice_log_level_t pLevel, const char* pMessage) {
            CLogger::instance()->log(QString("ICE: %1").arg(QString::fromUtf8(pMessage)),
                                     pLevel >= JUICE_LOG_LEVEL_ERROR ? Qt::red : Qt::darkMagenta,
                                     LEVEL_NORMAL);
        });
        return true;
    }();
    Q_UNUSED(logRouted);
}

PeerLink::~PeerLink()
{
    close();
}

bool PeerLink::start()
{
    close();
    const QByteArray stunHost = m_stun.host.toUtf8();
    const QByteArray turnHost = m_turn.host.toUtf8();
    const QByteArray turnUser = m_turn.username.toUtf8();
    const QByteArray turnPassword = m_turn.password.toUtf8();

    juice_config_t config{};
    config.concurrency_mode = JUICE_CONCURRENCY_MODE_POLL;
    if (!m_stun.host.isEmpty())
    {
        config.stun_server_host = stunHost.constData();
        config.stun_server_port = m_stun.port;
    }
    juice_turn_server_t turn{};
    if (!m_turn.host.isEmpty())
    {
        turn.host = turnHost.constData();
        turn.port = m_turn.port;
        turn.username = turnUser.constData();
        turn.password = turnPassword.constData();
        config.turn_servers = &turn;
        config.turn_servers_count = 1;
    }
    config.cb_state_changed = &Callbacks::stateChanged;
    config.cb_gathering_done = &Callbacks::gatheringDone;
    config.cb_recv = &Callbacks::received;
    config.user_ptr = this;

    // libjuice copies the configuration strings
    m_agent = juice_create(&config);
    if (!m_agent)
    {
        CLogger::instance()->log("PeerLink: cannot create the ICE agent", Qt::red);
        setState(State::Failed);
        return false;
    }
    m_remoteSet = false;
    m_pathType.clear();
    m_localDescription.clear();
    setState(State::Gathering);
    if (juice_gather_candidates(m_agent) != 0)
    {
        setState(State::Failed);
        return false;
    }
    return true;
}

void PeerLink::close()
{
    if (!m_agent)
        return;
    // Stops the libjuice thread: no callback after this
    juice_destroy(m_agent);
    m_agent = nullptr;
    m_pathType.clear();
    setState(State::Closed);
}

bool PeerLink::setRemoteDescription(const QString& pDescription)
{
    if (!m_agent || m_remoteSet)
        return false;
    if (juice_set_remote_description(m_agent, pDescription.toLatin1().constData()) != 0)
        return false;
    // The description holds all the candidates of the peer
    juice_set_remote_gathering_done(m_agent);
    m_remoteSet = true;
    if (m_state == State::Ready)
        setState(State::Connecting);
    return true;
}

bool PeerLink::send(const QByteArray& pDatagram)
{
    if (!m_agent || m_state != State::Connected)
        return false;
    return juice_send(m_agent, pDatagram.constData(), static_cast<size_t>(pDatagram.size())) == 0;
}

void PeerLink::setState(State pState)
{
    if (pState == m_state)
        return;
    m_state = pState;
    emit stateChanged(pState);
}
