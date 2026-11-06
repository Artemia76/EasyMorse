// Tests for core/net PeerSession: two sessions in one process exchange
// invitations and talk over the local network - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>

#include "keyer/monotonicclock.h"
#include "net/peersession.h"

class TestPeerSession : public QObject
{
    Q_OBJECT

private:
    static void connectPair(PeerSession& pAlice, PeerSession& pBob)
    {
        pAlice.setCallsign("F4IKZ");
        pBob.setCallsign("F5XYZ");
        pAlice.start();
        pBob.start();
        QTRY_COMPARE_WITH_TIMEOUT(pAlice.state(), PeerSession::State::Inviting, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(pBob.state(), PeerSession::State::Inviting, 5000);
        QVERIFY(pAlice.invitation().startsWith("EM1:"));
        // Each operator pastes the other's invitation
        QCOMPARE(pAlice.join(pBob.invitation()), QString());
        QCOMPARE(pBob.join(pAlice.invitation()), QString());
        QTRY_COMPARE_WITH_TIMEOUT(pAlice.state(), PeerSession::State::Connected, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(pBob.state(), PeerSession::State::Connected, 10000);
    }

private slots:
    void connectsAndPlaysTheKeyWithItsRhythm()
    {
        PeerSession alice, bob;
        bob.setJitterDelay(100);
        connectPair(alice, bob);
        QCOMPARE(bob.peerCallsign(), QString("F4IKZ"));
        QCOMPARE(alice.peerCallsign(), QString("F5XYZ"));
        QVERIFY(!alice.fingerprint().isEmpty());
        QCOMPARE(alice.fingerprint(), bob.fingerprint());

        // "A" at 20 WPM keyed by Alice
        QList<QPair<bool, qint64>> received;
        connect(bob.remoteKeyer(), &KeyerInput::keyChanged, this, [&](bool down, qint64 time) {
            received.append({down, time});
        });
        const QList<QPair<bool, int>> keyed = {{true, 60}, {false, 60}, {true, 180}, {false, 0}};
        for (const auto& [down, hold] : keyed)
        {
            alice.sendKey(down, MonotonicClock::nowMs());
            QTest::qWait(hold);
        }
        QTRY_COMPARE_WITH_TIMEOUT(received.size(), 4, 3000);
        QVERIFY(received[0].first && !received[1].first && received[2].first && !received[3].first);
        // Durations within a few ms (timers of the test itself are not exact)
        const qint64 dot = received[1].second - received[0].second;
        const qint64 dash = received[3].second - received[2].second;
        QVERIFY2(qAbs(dot - 60) < 25, qPrintable(QString::number(dot)));
        QVERIFY2(qAbs(dash - 180) < 25, qPrintable(QString::number(dash)));
    }

    void exchangesTextAndRoundTrip()
    {
        PeerSession alice, bob;
        connectPair(alice, bob);
        QSignalSpy text(&bob, &PeerSession::textReceived);
        QVERIFY(alice.sendText("RST 599"));
        QTRY_COMPARE_WITH_TIMEOUT(text.count(), 1, 3000);
        QCOMPARE(text.at(0).at(0).toString(), QString("RST 599"));
        QTRY_VERIFY_WITH_TIMEOUT(alice.roundTripMs() >= 0, 3000);
    }

    void peerLeavingReleasesTheKey()
    {
        PeerSession alice, bob;
        connectPair(alice, bob);
        bob.setJitterDelay(0);
        alice.sendKey(true, MonotonicClock::nowMs());
        QTRY_VERIFY_WITH_TIMEOUT(bob.remoteKeyer()->isDown(), 3000);
        alice.stop();
        QTRY_COMPARE_WITH_TIMEOUT(bob.state(), PeerSession::State::Disconnected, 3000);
        QVERIFY(!bob.remoteKeyer()->isDown());
    }

    void rejectsBadInvitations()
    {
        PeerSession alice;
        alice.start();
        QTRY_COMPARE_WITH_TIMEOUT(alice.state(), PeerSession::State::Inviting, 5000);
        QVERIFY(!alice.join("hello").isEmpty());
        QVERIFY(!alice.join("EM1:garbage").isEmpty());
        QVERIFY(!alice.join(alice.invitation()).isEmpty());
        QCOMPARE(alice.state(), PeerSession::State::Inviting);
    }
};

QTEST_GUILESS_MAIN(TestPeerSession)
#include "tst_peersession.moc"
