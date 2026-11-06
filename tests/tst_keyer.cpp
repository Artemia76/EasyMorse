// Tests for core/keyer - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>

#include "keyer/keyerinput.h"
#ifdef EASYMORSE_HAS_SERIALPORT
#include "keyer/serialkeyer.h"
#endif

class TestKeyer : public QObject
{
    Q_OBJECT

private slots:
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
