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
* turnrelay.h is part of EasyMorse server
*
* STUN and TURN server (libjuice): STUN tells clients their public address,
* TURN relays the sessions that cannot go direct (symmetric NAT on both
* sides). Each logged in user gets random credentials valid for a while.
* **************************************************************************/

#ifndef TURNRELAY_H
#define TURNRELAY_H

#include <QString>

struct juice_server;

class TurnRelay
{
public:
    struct Config
    {
        quint16 port = 3478;
        QString bindAddress;        // empty: all interfaces
        QString externalAddress;    // public address announced for relays
        quint16 relayPortBegin = 0; // 0: any port
        quint16 relayPortEnd = 0;
        int     maxAllocations = 100;
    };

    struct Credentials
    {
        QString username;
        QString password;
        int     lifetimeSeconds = 0;
    };

    static constexpr int CredentialsLifetimeSeconds = 12 * 3600;
    static constexpr int AllocationsPerUser = 4;

    TurnRelay() = default;
    ~TurnRelay();
    TurnRelay(const TurnRelay&) = delete;
    TurnRelay& operator=(const TurnRelay&) = delete;

    bool        start(const Config& pConfig);
    void        stop();
    bool        isRunning() const { return m_server != nullptr; }
    quint16     port() const;

    ///
    /// \brief new random credentials, accepted by the relay until they expire
    ///
    Credentials issueCredentials();

private:
    juice_server*   m_server = nullptr;
};

#endif // TURNRELAY_H
