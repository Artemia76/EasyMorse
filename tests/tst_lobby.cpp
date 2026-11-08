// End to end: match-making server in process, clients in a room opening
// peer to peer sessions with each other by themselves - part of EasyMorse, GPLv3

#include <QTest>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>

#include "keyer/monotonicclock.h"
#include "accounts.h"
#include "net/lobby.h"
#include "net/peerlink.h"
#include "net/peersession.h"
#include "server.h"
#include "turnrelay.h"

namespace
{
    struct Station
    {
        Lobby lobby;

        SignalingClient* client() { return lobby.client(); }
        PeerSession* peer(const QString& pCallsign) { return lobby.peer(pCallsign); }
        bool connectedWith(const QString& pCallsign)
        {
            PeerSession* session = lobby.peer(pCallsign);
            return session && session->state() == PeerSession::State::Connected;
        }
    };
}

class TestLobby : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<Server> m_server;

    QUrl serverUrl() const { return QUrl(QString("http://127.0.0.1:%1").arg(m_server->port())); }

    void enter(Station& pStation, const QString& pCallsign)
    {
        pStation.client()->setServerUrl(serverUrl());
        pStation.client()->registerAccount(pCallsign, "morse code!");
        QTRY_COMPARE_WITH_TIMEOUT(pStation.client()->state(), SignalingClient::State::Online, 10000);
        QCOMPARE(pStation.client()->callsign(), pCallsign);
        pStation.client()->joinRoom("QSO");
        QTRY_COMPARE_WITH_TIMEOUT(pStation.client()->room(), QString("QSO"), 3000);
    }

private slots:
    void init()
    {
        m_server = std::make_unique<Server>();
        Server::Config config;
        config.port = 0;
        config.database = ":memory:";
        QVERIFY(m_server->start(config));
    }

    void cleanup()
    {
        // Let the server free the connections the clients just closed
        QTest::qWait(100);
        m_server.reset();
    }

    void roomMembersConnectByThemselves()
    {
        Station alice, bob, carol;
        enter(alice, "F4IKZ");
        enter(bob, "F5XYZ");
        enter(carol, "F6CAR");
        QTRY_COMPARE_WITH_TIMEOUT(alice.client()->members().size(), 3, 3000);

        // Everyone with everyone, without calling
        QTRY_COMPARE_WITH_TIMEOUT(alice.lobby.connectedCount(), 2, 20000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.connectedCount(), 2, 20000);
        QTRY_COMPARE_WITH_TIMEOUT(carol.lobby.connectedCount(), 2, 20000);
        QCOMPARE(alice.lobby.peers().size(), 2);
        QVERIFY(!alice.peer("F4IKZ"));
        QCOMPARE(alice.peer("F5XYZ")->fingerprint(), bob.peer("F4IKZ")->fingerprint());
        QCOMPARE(bob.peer("F6CAR")->fingerprint(), carol.peer("F5XYZ")->fingerprint());
        QCOMPARE(bob.peer("F4IKZ")->peerCallsign(), QString("F4IKZ"));

        // Alice's key reaches both
        bob.lobby.setJitterDelay(0);
        carol.lobby.setJitterDelay(0);
        alice.lobby.sendKey(true, MonotonicClock::nowMs());
        QTRY_VERIFY_WITH_TIMEOUT(bob.peer("F4IKZ")->remoteKeyer()->isDown(), 3000);
        QTRY_VERIFY_WITH_TIMEOUT(carol.peer("F4IKZ")->remoteKeyer()->isDown(), 3000);
        alice.lobby.sendKey(false, MonotonicClock::nowMs());
        QTRY_VERIFY_WITH_TIMEOUT(!bob.peer("F4IKZ")->remoteKeyer()->isDown(), 3000);

        // Carol leaves: her sessions close, the others stay
        QSignalSpy removed(&alice.lobby, &Lobby::peerRemoved);
        carol.client()->leaveRoom();
        QCOMPARE(carol.lobby.peers().size(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(alice.lobby.peers().size(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.peers().size(), 1, 3000);
        QCOMPARE(removed.count(), 1);
        QVERIFY(alice.connectedWith("F5XYZ"));
        QVERIFY(bob.connectedWith("F4IKZ"));

        // Logging out closes everything
        alice.client()->logout();
        QCOMPARE(alice.lobby.peers().size(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.peers().size(), 0, 3000);
    }

    void lostSessionIsOpenedAgain()
    {
        Station alice, bob;
        enter(alice, "F4IKZ");
        enter(bob, "F5XYZ");
        QTRY_VERIFY_WITH_TIMEOUT(alice.connectedWith("F5XYZ"), 20000);
        QTRY_VERIFY_WITH_TIMEOUT(bob.connectedWith("F4IKZ"), 20000);

        // Bob's side of the link breaks while both stay in the room
        bob.peer("F4IKZ")->stop();
        QTRY_VERIFY_WITH_TIMEOUT(!alice.connectedWith("F5XYZ"), 5000);
        // Alice (first callsign) starts again after a short delay
        QTRY_VERIFY_WITH_TIMEOUT(alice.connectedWith("F5XYZ"), 30000);
        QTRY_VERIFY_WITH_TIMEOUT(bob.connectedWith("F4IKZ"), 10000);
        QCOMPARE(alice.peer("F5XYZ")->fingerprint(), bob.peer("F4IKZ")->fingerprint());
    }

    void suspendedAccountIsDisconnected()
    {
        Station alice;
        QSignalSpy token(alice.client(), &SignalingClient::tokenChanged);
        enter(alice, "F4IKZ");
        QSignalSpy error(alice.client(), &SignalingClient::errorOccurred);

        // As the administration page does
        QNetworkAccessManager network;
        QString adminToken;
        QVERIFY(m_server->accounts().registerUser("F1ADM", "admin password", adminToken) == Accounts::Result::Ok);
        QVERIFY(m_server->accounts().setAdmin("F1ADM", true) == Accounts::Result::Ok);
        QNetworkRequest request(QUrl(serverUrl().toString() + "/api/v1/admin/users"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + adminToken.toLatin1());
        QNetworkReply* reply = network.post(request, R"({"action":"ban","callsign":"F4IKZ","reason":"spam"})");
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QVERIFY(finished.wait(5000));
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 204);
        reply->deleteLater();

        QTRY_VERIFY_WITH_TIMEOUT(error.count() > 0, 3000);
        QVERIFY(error.at(0).at(0).toString().contains("spam"));
        QTRY_COMPARE_WITH_TIMEOUT(alice.client()->state(), SignalingClient::State::Offline, 3000);
        // The saved token is forgotten: no reconnection
        QVERIFY(token.last().at(0).toString().isEmpty());
        network.clearConnectionCache();
    }

    void relayCredentialsWork()
    {
        // The credentials issued by the server open a relay on its TURN server
        TurnRelay relay;
        TurnRelay::Config config;
        config.port = 0;
        config.bindAddress = "127.0.0.1";
        config.externalAddress = "127.0.0.1";
        QVERIFY(relay.start(config));
        const TurnRelay::Credentials credentials = relay.issueCredentials();
        QVERIFY(!credentials.username.isEmpty());

        PeerLink link;
        link.setTurnServer({"127.0.0.1", relay.port(), credentials.username, credentials.password});
        QSignalSpy gathered(&link, &PeerLink::gatheringDone);
        QVERIFY(link.start());
        QVERIFY(gathered.wait(8000));
        QVERIFY2(link.localDescription().contains(" typ relay "), qPrintable(link.localDescription()));

        // Wrong credentials: no relay
        PeerLink intruder;
        intruder.setTurnServer({"127.0.0.1", relay.port(), credentials.username, "wrong"});
        QSignalSpy intruderGathered(&intruder, &PeerLink::gatheringDone);
        QVERIFY(intruder.start());
        QVERIFY(intruderGathered.wait(8000));
        QVERIFY(!intruder.localDescription().contains(" typ relay "));
    }

    void resumesWithTheSavedToken()
    {
        Station alice;
        QSignalSpy token(alice.client(), &SignalingClient::tokenChanged);
        enter(alice, "F4IKZ");
        QCOMPARE(token.count(), 1);
        const QString saved = token.at(0).at(0).toString();

        Station again;
        again.client()->setServerUrl(serverUrl());
        again.client()->resume(saved);
        QTRY_COMPARE_WITH_TIMEOUT(again.client()->state(), SignalingClient::State::Online, 5000);
        QCOMPARE(again.client()->callsign(), QString("F4IKZ"));
        // The older connection was replaced
        QTRY_COMPARE_WITH_TIMEOUT(alice.client()->state(), SignalingClient::State::Offline, 5000);

        again.client()->logout();
        // The logout request is asynchronous: wait for the server to revoke the token
        const auto tokenAccepted = [&] {
            QNetworkAccessManager network;
            QNetworkRequest request(QUrl(serverUrl().toString() + "/api/v1/me"));
            request.setRawHeader("Authorization", "Bearer " + saved.toLatin1());
            std::unique_ptr<QNetworkReply> reply(network.get(request));
            QSignalSpy finished(reply.get(), &QNetworkReply::finished);
            finished.wait(2000);
            return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 401;
        };
        QTRY_VERIFY_WITH_TIMEOUT(!tokenAccepted(), 5000);
        Station third;
        third.client()->setServerUrl(serverUrl());
        QSignalSpy error(third.client(), &SignalingClient::errorOccurred);
        third.client()->resume(saved);
        QVERIFY(error.wait(5000));
        QCOMPARE(third.client()->state(), SignalingClient::State::Offline);
    }

    // A server which went away must not hold the login form forever
    void unreachableServerCanBeLeft()
    {
        SignalingClient client;
        QSignalSpy token(&client, &SignalingClient::tokenChanged);
        const QUrl closed(QStringLiteral("http://127.0.0.1:9"));
        client.setServerUrl(closed);

        // Another address: the token of the old server is forgotten
        client.resume("old-token");
        QCOMPARE(client.state(), SignalingClient::State::Connecting);
        QTest::qWait(200);
        client.setServerUrl(closed);    // same address: still trying
        QCOMPARE(client.state(), SignalingClient::State::Connecting);
        client.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:7")));
        QCOMPARE(client.state(), SignalingClient::State::Offline);
        QCOMPARE(token.count(), 1);
        QVERIFY(token.first().first().toString().isEmpty());

        // Giving up by hand
        client.resume("old-token");
        QCOMPARE(client.state(), SignalingClient::State::Connecting);
        client.logout();
        QCOMPARE(client.state(), SignalingClient::State::Offline);

        // A login takes over from the reconnection, which does not come back
        client.resume("old-token");
        QSignalSpy error(&client, &SignalingClient::errorOccurred);
        client.login("F4ABC", "password123");
        QCOMPARE(client.state(), SignalingClient::State::LoggingIn);
        QVERIFY(error.wait(5000));
        QTest::qWait(2500);
        QCOMPARE(client.state(), SignalingClient::State::Offline);
    }
};

QTEST_GUILESS_MAIN(TestLobby)
#include "tst_lobby.moc"
