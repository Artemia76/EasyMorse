// Tests for core/net PeerLink: two links in the same process connect over
// the local network (host candidates, no STUN server) - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>

#include "net/peerlink.h"

class TestPeerLink : public QObject
{
    Q_OBJECT

private slots:
    void connectsAndExchangesDatagrams()
    {
        PeerLink alice, bob;
        QSignalSpy aliceReady(&alice, &PeerLink::gatheringDone);
        QSignalSpy bobReady(&bob, &PeerLink::gatheringDone);
        QVERIFY(alice.start());
        QVERIFY(bob.start());
        QVERIFY(aliceReady.wait(5000) || aliceReady.count() == 1);
        QTRY_COMPARE_WITH_TIMEOUT(bobReady.count(), 1, 5000);
        QCOMPARE(alice.state(), PeerLink::State::Ready);
        QVERIFY(alice.localDescription().contains("a=ice-ufrag"));

        // The descriptions travel through the match-making server in real use
        QVERIFY(alice.setRemoteDescription(bob.localDescription()));
        QVERIFY(bob.setRemoteDescription(alice.localDescription()));
        QTRY_COMPARE_WITH_TIMEOUT(alice.state(), PeerLink::State::Connected, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.state(), PeerLink::State::Connected, 10000);
        QCOMPARE(alice.pathType(), QString("host"));

        QSignalSpy bobReceived(&bob, &PeerLink::datagramReceived);
        QSignalSpy aliceReceived(&alice, &PeerLink::datagramReceived);
        QVERIFY(alice.send("CQ CQ DE F4IKZ"));
        QVERIFY(bob.send("F4IKZ DE F5XYZ"));
        QTRY_COMPARE_WITH_TIMEOUT(bobReceived.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(aliceReceived.count(), 1, 3000);
        QCOMPARE(bobReceived.at(0).at(0).toByteArray(), QByteArray("CQ CQ DE F4IKZ"));
        QCOMPARE(aliceReceived.at(0).at(0).toByteArray(), QByteArray("F4IKZ DE F5XYZ"));

        alice.close();
        QCOMPARE(alice.state(), PeerLink::State::Closed);
        QVERIFY(!alice.send("lost"));
    }

    void rejectsGarbageDescription()
    {
        PeerLink link;
        QSignalSpy ready(&link, &PeerLink::gatheringDone);
        QVERIFY(link.start());
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QVERIFY(!link.setRemoteDescription("this is not SDP"));
    }
};

QTEST_GUILESS_MAIN(TestPeerLink)
#include "tst_peerlink.moc"
