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
* main.cpp is part of EasyMorse server
*
* easymorse-server: match-making server for EasyMorse. Run it behind a
* reverse proxy doing TLS (see server/deploy).
* **************************************************************************/

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>

#include <cstdio>

#include "server.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("easymorse-server");
    QCoreApplication::setApplicationVersion(QStringLiteral(EASYMORSE_SERVER_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription("EasyMorse match-making server");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption listen("listen", "HTTP and WebSocket address (default 127.0.0.1).", "address", "127.0.0.1");
    const QCommandLineOption port("port", "HTTP and WebSocket port (default 8080).", "port", "8080");
    const QCommandLineOption database("database", "SQLite database file.", "file", "easymorse.db");
    const QCommandLineOption trustProxy("trust-proxy", "Read client addresses from X-Forwarded-For.");
    const QCommandLineOption rooms("rooms", "Permanent rooms, comma separated.", "names", "Beginners,QSO");
    const QCommandLineOption publicHost("public-host", "Host name given to clients for STUN/TURN; enables the relay.", "host");
    const QCommandLineOption turnPort("turn-port", "STUN/TURN UDP port (default 3478).", "port", "3478");
    const QCommandLineOption externalAddress("external-address", "Public IP address announced for relayed sessions.", "address");
    const QCommandLineOption relayPorts("relay-ports", "UDP port range for relayed sessions, e.g. 49160-49200.", "range");
    parser.addOptions({listen, port, database, trustProxy, rooms, publicHost, turnPort, externalAddress, relayPorts});
    parser.process(app);

    Server::Config config;
    config.address = QHostAddress(parser.value(listen));
    config.port = static_cast<quint16>(parser.value(port).toUInt());
    config.database = parser.value(database);
    config.trustProxy = parser.isSet(trustProxy);
    config.permanentRooms = parser.value(rooms).split(u',', Qt::SkipEmptyParts);
    config.publicHost = parser.value(publicHost);
    config.turnEnabled = !config.publicHost.isEmpty();
    config.turn.port = static_cast<quint16>(parser.value(turnPort).toUInt());
    config.turn.externalAddress = parser.value(externalAddress);
    const QStringList range = parser.value(relayPorts).split(u'-');
    if (range.size() == 2)
    {
        config.turn.relayPortBegin = static_cast<quint16>(range[0].toUInt());
        config.turn.relayPortEnd = static_cast<quint16>(range[1].toUInt());
    }
    if (config.address.isNull())
    {
        std::fprintf(stderr, "Invalid listen address\n");
        return 2;
    }

    Server server;
    if (!server.start(config))
        return 1;
    std::printf("easymorse-server %s listening on %s:%u%s\n", EASYMORSE_SERVER_VERSION,
                qPrintable(config.address.toString()), server.port(),
                config.turnEnabled ? qPrintable(QString(", STUN/TURN on UDP %1").arg(config.turn.port)) : "");
    std::fflush(stdout);
    return app.exec();
}
