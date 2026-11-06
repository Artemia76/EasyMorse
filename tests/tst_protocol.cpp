// Tests for core/net protocol and secure channel - part of EasyMorse, GPLv3

#include <QTest>

#include "net/protocol.h"
#include "net/securechannel.h"

using namespace Protocol;

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip_data()
    {
        QTest::addColumn<Packet>("packet");
        QTest::newRow("hello") << Packet(Hello{"F4IKZ"});
        QTest::newRow("keys") << Packet(Keys{{{1, 100, true}, {2, 160, false}, {3, 220, true}}});
        QTest::newRow("text") << Packet(Text{7, "73 de F4IKZ — à bientôt"});
        QTest::newRow("ping") << Packet(Ping{42});
        QTest::newRow("pong") << Packet(Pong{42});
        QTest::newRow("bye") << Packet(Bye{});
    }

    void roundTrip()
    {
        QFETCH(Packet, packet);
        const auto decoded = decode(encode(packet));
        QVERIFY(decoded.has_value());
        QVERIFY(*decoded == packet);
    }

    void keysKeepTheLastEvents()
    {
        Keys keys;
        for (quint32 i = 0; i < 20; ++i)
            keys.events.append({i, i * 50, (i % 2) == 0});
        const auto decoded = decode(encode(keys));
        QVERIFY(decoded.has_value());
        const auto& events = std::get<Keys>(*decoded).events;
        QCOMPARE(events.size(), KeyEventsPerPacket);
        QCOMPARE(events.last().sequence, 19u);
    }

    void longTextIsCutOnACharacter()
    {
        const QString text = QString(MaxTextBytes - 1, u'a') + "é";
        const auto decoded = decode(encode(Text{1, text}));
        QVERIFY(decoded.has_value());
        QCOMPARE(std::get<Text>(*decoded).text, QString(MaxTextBytes - 1, u'a'));
    }

    void rejectsInvalidData()
    {
        QVERIFY(!decode(QByteArray()).has_value());
        QByteArray wrongVersion = encode(Ping{1});
        wrongVersion[0] = 99;
        QVERIFY(!decode(wrongVersion).has_value());
        QByteArray truncated = encode(Keys{{{1, 2, true}}});
        truncated.chop(1);
        QVERIFY(!decode(truncated).has_value());
        QVERIFY(!decode(encode(Ping{1}) + "x").has_value());
        QByteArray unknown = encode(Bye{});
        unknown[1] = 77;
        QVERIFY(!decode(unknown).has_value());
    }

    void channelsTalkBothWays()
    {
        SecureChannel alice, bob;
        QVERIFY(alice.setPeerPublicKey(bob.publicKey()));
        QVERIFY(bob.setPeerPublicKey(alice.publicKey()));
        QCOMPARE(alice.fingerprint(), bob.fingerprint());
        QCOMPARE(alice.fingerprint().size(), 19);

        const QByteArray sealed = alice.seal("CQ CQ");
        QCOMPARE(sealed.size(), 5 + SecureChannel::Overhead);
        QVERIFY(!sealed.contains("CQ CQ"));
        QCOMPARE(bob.open(sealed).value_or("?"), QByteArray("CQ CQ"));
        QCOMPARE(alice.open(bob.seal("73")).value_or("?"), QByteArray("73"));
        // A packet sealed for the other direction is not accepted back
        QVERIFY(!alice.open(alice.seal("echo")).has_value());
    }

    void rejectsForgedAndReplayedPackets()
    {
        SecureChannel alice, bob;
        alice.setPeerPublicKey(bob.publicKey());
        bob.setPeerPublicKey(alice.publicKey());

        const QByteArray first = alice.seal("one");
        const QByteArray second = alice.seal("two");
        QByteArray forged = second;
        forged[10] = forged[10] ^ 1;
        QVERIFY(!bob.open(forged).has_value());

        // Out of order is fine, replay is not
        QVERIFY(bob.open(second).has_value());
        QVERIFY(bob.open(first).has_value());
        QVERIFY(!bob.open(first).has_value());
        QVERIFY(!bob.open(second).has_value());

        // Far too old
        QByteArray old = alice.seal("old");
        for (int i = 0; i < 70; ++i)
            QVERIFY(bob.open(alice.seal("new")).has_value());
        QVERIFY(!bob.open(old).has_value());
    }

    void otherSessionCannotRead()
    {
        SecureChannel alice, bob, eve;
        alice.setPeerPublicKey(bob.publicKey());
        bob.setPeerPublicKey(alice.publicKey());
        eve.setPeerPublicKey(alice.publicKey());
        QVERIFY(!eve.open(alice.seal("secret")).has_value());
        QVERIFY(eve.fingerprint() != bob.fingerprint());
    }

    void rejectsInvalidKeys()
    {
        SecureChannel alice;
        QVERIFY(!alice.setPeerPublicKey(QByteArray(31, 'x')));
        QVERIFY(!alice.setPeerPublicKey(alice.publicKey()));
        // Low order point: all zero
        QVERIFY(!alice.setPeerPublicKey(QByteArray(32, '\0')));
        QVERIFY(!alice.isReady());
        QVERIFY(alice.seal("x").isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestProtocol)
#include "tst_protocol.moc"
