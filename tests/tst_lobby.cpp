// End to end: match-making server in process, two clients calling each
// other and opening a peer to peer session - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>

#include "keyer/monotonicclock.h"
#include "net/lobby.h"
#include "net/peersession.h"
#include "server.h"

namespace
{
    struct Station
    {
        PeerSession session;
        Lobby       lobby{&session};

        SignalingClient* client() { return lobby.client(); }
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
        m_server.reset();
    }

    void callOpensASessionWithoutCopyAndPaste()
    {
        Station alice, bob;
        enter(alice, "F4IKZ");
        enter(bob, "F5XYZ");
        QTRY_COMPARE_WITH_TIMEOUT(alice.client()->members(), QStringList({"F4IKZ", "F5XYZ"}), 3000);

        QSignalSpy ringing(&bob.lobby, &Lobby::incomingCall);
        QVERIFY(alice.lobby.call("F5XYZ"));
        QCOMPARE(alice.lobby.callState(), Lobby::CallState::Calling);
        QVERIFY(ringing.wait(3000));
        QCOMPARE(ringing.at(0).at(0).toString(), QString("F4IKZ"));
        QCOMPARE(bob.lobby.callState(), Lobby::CallState::Ringing);

        bob.lobby.accept();
        QTRY_COMPARE_WITH_TIMEOUT(alice.session.state(), PeerSession::State::Connected, 15000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.session.state(), PeerSession::State::Connected, 15000);
        QCOMPARE(alice.lobby.callState(), Lobby::CallState::InCall);
        QCOMPARE(alice.session.fingerprint(), bob.session.fingerprint());

        // Morse goes peer to peer
        bob.session.setJitterDelay(0);
        alice.session.sendKey(true, MonotonicClock::nowMs());
        QTRY_VERIFY_WITH_TIMEOUT(bob.session.remoteKeyer()->isDown(), 3000);

        alice.lobby.hangup();
        QCOMPARE(alice.lobby.callState(), Lobby::CallState::Idle);
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.callState(), Lobby::CallState::Idle, 3000);
        QVERIFY(!bob.session.remoteKeyer()->isDown());
    }

    void declineAndBusy()
    {
        Station alice, bob, carol;
        enter(alice, "F4IKZ");
        enter(bob, "F5XYZ");
        enter(carol, "F6CAR");
        QTRY_COMPARE_WITH_TIMEOUT(alice.client()->members().size(), 3, 3000);

        QSignalSpy failed(&alice.lobby, &Lobby::callFailed);
        QVERIFY(alice.lobby.call("F5XYZ"));
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.callState(), Lobby::CallState::Ringing, 3000);
        bob.lobby.decline();
        QVERIFY(failed.wait(3000));
        QCOMPARE(failed.at(0).at(1).toString(), QString("declined"));
        QCOMPARE(alice.lobby.callState(), Lobby::CallState::Idle);

        // Bob is being called by Carol: Alice gets "busy"
        QVERIFY(carol.lobby.call("F5XYZ"));
        QTRY_COMPARE_WITH_TIMEOUT(bob.lobby.callState(), Lobby::CallState::Ringing, 3000);
        QVERIFY(alice.lobby.call("F5XYZ"));
        QVERIFY(failed.wait(3000));
        QCOMPARE(failed.at(1).at(1).toString(), QString("busy"));
        QCOMPARE(bob.lobby.callPeer(), QString("F6CAR"));

        // Cannot call oneself or someone absent
        QVERIFY(!alice.lobby.call("F4IKZ"));
        QVERIFY(!alice.lobby.call("NOBODY"));
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
        Station third;
        third.client()->setServerUrl(serverUrl());
        QSignalSpy error(third.client(), &SignalingClient::errorOccurred);
        third.client()->resume(saved);
        QVERIFY(error.wait(5000));
        QCOMPARE(third.client()->state(), SignalingClient::State::Offline);
    }
};

QTEST_GUILESS_MAIN(TestLobby)
#include "tst_lobby.moc"
