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
* turnrelay.cpp is part of EasyMorse server
* **************************************************************************/

#include "turnrelay.h"

#include <QByteArray>
#include <QRandomGenerator>

#include <juice/juice.h>

namespace
{
    QString randomText(int pBytes)
    {
        QByteArray bytes(pBytes, Qt::Uninitialized);
        QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(bytes.data()), pBytes / 4);
        return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }
}

TurnRelay::~TurnRelay()
{
    stop();
}

bool TurnRelay::start(const Config& pConfig)
{
    stop();
    const QByteArray bind = pConfig.bindAddress.toUtf8();
    const QByteArray external = pConfig.externalAddress.toUtf8();
    juice_server_config_t config{};
    config.port = pConfig.port;
    config.bind_address = bind.isEmpty() ? nullptr : bind.constData();
    config.external_address = external.isEmpty() ? nullptr : external.constData();
    config.relay_port_range_begin = pConfig.relayPortBegin;
    config.relay_port_range_end = pConfig.relayPortEnd;
    config.max_allocations = pConfig.maxAllocations;
    config.realm = "easymorse";
    // No static credentials: they are issued to logged in users
    m_server = juice_server_create(&config);
    return m_server != nullptr;
}

void TurnRelay::stop()
{
    if (m_server)
    {
        juice_server_destroy(m_server);
        m_server = nullptr;
    }
}

quint16 TurnRelay::port() const
{
    return m_server ? juice_server_get_port(m_server) : 0;
}

TurnRelay::Credentials TurnRelay::issueCredentials()
{
    if (!m_server)
        return {};
    Credentials credentials{randomText(12), randomText(24), CredentialsLifetimeSeconds};
    const QByteArray username = credentials.username.toLatin1();
    const QByteArray password = credentials.password.toLatin1();
    juice_server_credentials_t entry{username.constData(), password.constData(), AllocationsPerUser};
    // libjuice copies the strings
    if (juice_server_add_credentials(m_server, &entry,
                                     static_cast<unsigned long>(CredentialsLifetimeSeconds) * 1000UL) != 0)
        return {};
    return credentials;
}
