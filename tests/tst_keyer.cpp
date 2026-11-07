// Tests for core/keyer - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>

#include "keyer/iambickeyer.h"
#include "keyer/keyerinput.h"
#ifdef EASYMORSE_HAS_SERIALPORT
#include "keyer/serialkeyer.h"
#endif

namespace
{
    ///
    /// \brief key changes as "D0 U60 D120…" (down/up and time)
    ///
    QString edges(const QSignalSpy& pSpy)
    {
        QStringList list;
        for (const QList<QVariant>& change : pSpy)
            list.append(QString("%1%2").arg(change.at(0).toBool() ? "D" : "U").arg(change.at(1).toLongLong()));
        return list.join(u' ');
    }

    ///
    /// \brief keyer driven by a simulated clock, 20 WPM: a dot is 60 ms
    ///
    struct Keyer
    {
        IambicKeyer keyer;
        QSignalSpy  spy{&keyer, &KeyerInput::keyChanged};

        explicit Keyer(IambicKeyer::Mode pMode = IambicKeyer::Mode::B)
        {
            keyer.setAutoTimer(false);
            keyer.setSpeed(20);
            keyer.setMode(pMode);
        }
    };
}

class TestKeyer : public QObject
{
    Q_OBJECT

private slots:
    void heldDotPaddleSendsDots()
    {
        Keyer k;
        k.keyer.setPaddles(true, false, 0);
        k.keyer.advanceTo(250);
        k.keyer.setPaddles(false, false, 250);
        k.keyer.advanceTo(1000);
        // Dot, one dot of silence, again; the dot started finishes
        QCOMPARE(edges(k.spy), QString("D0 U60 D120 U180 D240 U300"));
        QVERIFY(!k.keyer.isDown());
    }

    void heldDashPaddleSendsDashes()
    {
        Keyer k;
        k.keyer.setPaddles(false, true, 0);
        k.keyer.advanceTo(500);
        k.keyer.setPaddles(false, false, 500);
        k.keyer.advanceTo(1000);
        QCOMPARE(edges(k.spy), QString("D0 U180 D240 U420 D480 U660"));
    }

    void squeezeAlternatesAndModesEndDifferently()
    {
        for (const IambicKeyer::Mode mode : {IambicKeyer::Mode::A, IambicKeyer::Mode::B})
        {
            Keyer k(mode);
            // Both closed: dot first, then alternating
            k.keyer.setPaddles(true, true, 0);
            k.keyer.advanceTo(500);
            // Squeeze released during the second dash
            k.keyer.setPaddles(false, false, 500);
            k.keyer.advanceTo(2000);
            const QString common = "D0 U60 D120 U300 D360 U420 D480 U660";
            // Mode A stops, mode B sends one more element, the opposite one
            QCOMPARE(edges(k.spy), mode == IambicKeyer::Mode::A ? common : common + " D720 U780");
        }
    }

    void contactTouchedDuringAnElementIsRemembered()
    {
        for (const IambicKeyer::Mode mode : {IambicKeyer::Mode::A, IambicKeyer::Mode::B})
        {
            Keyer k(mode);
            k.keyer.setPaddles(true, false, 0);
            k.keyer.setPaddles(false, false, 20);
            // Dash tapped and released during the dot, not squeezed
            k.keyer.setPaddles(false, true, 30);
            k.keyer.setPaddles(false, false, 45);
            k.keyer.advanceTo(1000);
            QCOMPARE(edges(k.spy), QString("D0 U60 D120 U300"));
        }
    }

    void followsTheSpeed()
    {
        Keyer k;
        k.keyer.setSpeed(10);
        k.keyer.setPaddles(true, false, 0);
        k.keyer.setPaddles(false, false, 10);
        k.keyer.advanceTo(1000);
        QCOMPARE(edges(k.spy), QString("D0 U120"));
        QCOMPARE(k.keyer.speed(), 10.0);
    }

    void resetReleasesTheKey()
    {
        Keyer k;
        k.keyer.setPaddles(false, true, 0);
        QVERIFY(k.keyer.isDown());
        k.keyer.reset(50);
        QVERIFY(!k.keyer.isDown());
        k.keyer.advanceTo(1000);
        QCOMPARE(edges(k.spy), QString("D0 U50"));
    }

    void realTimeKeying()
    {
        // With its own timer, at 30 WPM (40 ms dots)
        IambicKeyer keyer;
        keyer.setSpeed(30);
        QSignalSpy spy(&keyer, &KeyerInput::keyChanged);
        const qint64 start = MonotonicClock::nowMs();
        keyer.setPaddles(true, false, start);
        QTest::qWait(150);
        keyer.setPaddles(false, false, MonotonicClock::nowMs());
        QTRY_VERIFY_WITH_TIMEOUT(!keyer.isDown() && spy.count() >= 4, 1000);
        // Elements at their exact times, whatever the timer precision
        QCOMPARE(spy.at(1).at(1).toLongLong() - start, 40);
        QCOMPARE(spy.at(2).at(1).toLongLong() - start, 80);
    }

    void manualKeyerEmitsOnlyChanges()
    {
        ManualKeyer keyer;
        QSignalSpy spy(&keyer, &KeyerInput::keyChanged);
        const qint64 before = MonotonicClock::nowMs();
        keyer.press();
        keyer.press();      // keyboard auto repeat
        QVERIFY(keyer.isDown());
        keyer.release();
        keyer.release();
        QVERIFY(!keyer.isDown());
        const qint64 after = MonotonicClock::nowMs();

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(0).at(0).toBool(), true);
        QCOMPARE(spy.at(1).at(0).toBool(), false);
        const qint64 down = spy.at(0).at(1).toLongLong();
        const qint64 up = spy.at(1).at(1).toLongLong();
        QVERIFY(before <= down && down <= up && up <= after);
    }

    void clockIsMonotonic()
    {
        qint64 previous = MonotonicClock::nowMs();
        for (int i = 0; i < 1000; ++i)
        {
            const qint64 now = MonotonicClock::nowMs();
            QVERIFY(now >= previous);
            previous = now;
        }
    }

#ifdef EASYMORSE_HAS_SERIALPORT
    void serialKeyerRejectsUnknownPort()
    {
        SerialKeyer keyer;
        QSignalSpy errors(&keyer, &SerialKeyer::errorOccurred);
        QVERIFY(!keyer.open("EasyMorseNoSuchPort"));
        QCOMPARE(errors.count(), 1);
        QVERIFY(!keyer.isOpen());
        QVERIFY(keyer.portName().isEmpty());
        QVERIFY(!keyer.isDown());
        QVERIFY(!keyer.open("/dev/easymorse-no-such-device"));
        QCOMPARE(errors.count(), 2);
    }
#endif
};

QTEST_GUILESS_MAIN(TestKeyer)
#include "tst_keyer.moc"
