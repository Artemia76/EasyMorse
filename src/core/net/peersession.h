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
* peersession.h is part of EasyMorse project
*
* Morse session with one remote station: invitation codes, encrypted link,
* key events sent and played back with their rhythm, text messages,
* round trip time and connection loss detection.
* **************************************************************************/

#ifndef PEERSESSION_H
#define PEERSESSION_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

#include <memory>

#include "net/jitterbuffer.h"
#include "net/peerlink.h"
#include "net/protocol.h"
#include "net/remotekeyer.h"

class SecureChannel;

class PeerSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString invitation READ invitation NOTIFY invitationChanged)
    Q_PROPERTY(QString peerCallsign READ peerCallsign NOTIFY peerChanged)
    Q_PROPERTY(QString fingerprint READ fingerprint NOTIFY peerChanged)
    Q_PROPERTY(QString pathType READ pathType NOTIFY stateChanged)
    Q_PROPERTY(int roundTripMs READ roundTripMs NOTIFY roundTripChanged)
    Q_PROPERTY(KeyerInput* remoteKey READ remoteKeyInput CONSTANT)

public:
    enum class State
    {
        Idle,
        Preparing,      // gathering addresses for the invitation
        Inviting,       // invitation ready, waiting for the peer's one
        Connecting,     // until the peer proves it has our invitation
        Connected,
        Disconnected,   // the peer left or the link was lost
        Failed          // no path between the two stations
    };
    Q_ENUM(State)

    static constexpr int PingIntervalMs = 1000;
    static constexpr int TimeoutMs = 10000;

    explicit PeerSession(QObject* parent = nullptr);
    ~PeerSession() override;

    void    setCallsign(const QString& pCallsign) { m_callsign = pCallsign; }
    void    setStunServer(const PeerLink::Server& pServer) { m_link.setStunServer(pServer); }
    void    setTurnServer(const PeerLink::Server& pServer) { m_link.setTurnServer(pServer); }
    void    setJitterDelay(int pMs) { m_jitter.setDelay(pMs); }

    ///
    /// \brief the remote station key, played back with its rhythm
    ///
    RemoteKeyer* remoteKeyer() { return &m_remoteKeyer; }
    const RemoteKeyer* remoteKeyer() const { return &m_remoteKeyer; }
    KeyerInput* remoteKeyInput() { return &m_remoteKeyer; }

    State   state() const { return m_state; }
    QString invitation() const { return m_invitation; }
    QString peerCallsign() const { return m_peerCallsign; }
    QString fingerprint() const;
    QString pathType() const { return m_link.pathType(); }
    int     roundTripMs() const { return m_roundTripMs; }

    ///
    /// \brief prepare an invitation code to give to the peer
    ///
    Q_INVOKABLE void start();
    ///
    /// \brief connect with the invitation code given by the peer
    /// \return an error message, empty on success
    ///
    Q_INVOKABLE QString join(const QString& pInvitation);
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool sendText(const QString& pText);

public slots:
    ///
    /// \brief send a change of the local key (MonotonicClock time)
    ///
    void    sendKey(bool pDown, qint64 pTimeMs);

signals:
    void    stateChanged();
    void    invitationChanged();
    void    peerChanged();
    void    roundTripChanged();
    void    textReceived(const QString& pText);

private:
    void    setState(State pState);
    void    onLinkState(PeerLink::State pState);
    void    onDatagram(const QByteArray& pDatagram);
    void    sendPacket(const Protocol::Packet& pPacket);
    void    sendKeys();
    void    ping();
    void    endSession(State pState);
    void    peerConfirmed();

    PeerLink                        m_link;
    std::unique_ptr<SecureChannel>  m_channel;
    JitterBuffer                    m_jitter;
    RemoteKeyer                     m_remoteKeyer;
    QTimer                          m_pingTimer;
    QTimer                          m_helloTimer;
    State                           m_state = State::Idle;
    QString                         m_callsign;
    QString                         m_invitation;
    QString                         m_peerCallsign;
    QString                         m_pendingInvitation;
    qint64                          m_startMs = 0;
    qint64                          m_lastReceivedMs = 0;
    // Outgoing key events, the last ones are repeated in every packet
    QList<Protocol::KeyEvent>       m_keyEvents;
    quint32                         m_nextKeySequence = 0;
    quint32                         m_nextPingId = 0;
    QHash<quint32, qint64>          m_pingsSent;
    int                             m_roundTripMs = -1;
    quint32                         m_nextTextId = 0;
};

#endif // PEERSESSION_H
