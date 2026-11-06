// End to end playback through AudioEngine + MorseSequencer on miniaudio's
// silent "null" device - part of EasyMorse, GPLv3

#include <QTest>

#include <QElapsedTimer>
#include <QSignalSpy>

#include "audio/AudioEngine.h"
#include "morse/sequencer.h"

class TestPlayback : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qputenv("EASYMORSE_AUDIO_BACKEND", "null");
    }

    void playsInRealTimeWithoutBlocking()
    {
        AudioEngine audio;
        QVERIFY(audio.start());
        MorseSequencer sequencer(&audio);
        QSignalSpy chars(&sequencer, &MorseSequencer::charPlayed);
        QSignalSpy finished(&sequencer, &MorseSequencer::finished);

        // "EE" at 60 WPM: 20 ms dots, 60 ms character gap -> 160 ms
        const MorseTiming timing = MorseTiming::fromSpeed(60, 60, false);
        QElapsedTimer clock;
        clock.start();
        QVERIFY(sequencer.play("EE", timing));
        QVERIFY2(clock.elapsed() < 50, "play() must return immediately");
        QVERIFY(sequencer.isPlaying());

        QVERIFY(finished.wait(3000));
        const qint64 elapsed = clock.elapsed();
        QVERIFY2(elapsed >= 150, qPrintable(QString("finished too early: %1 ms").arg(elapsed)));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(chars.count(), 2);
        QCOMPARE(chars.at(0).at(0).toInt(), 0);
        QCOMPARE(chars.at(1).at(0).toInt(), 1);
        QVERIFY(!sequencer.isPlaying());
    }

    void stopCancelsAtOnce()
    {
        AudioEngine audio;
        QVERIFY(audio.start());
        MorseSequencer sequencer(&audio);
        QSignalSpy chars(&sequencer, &MorseSequencer::charPlayed);
        QSignalSpy finished(&sequencer, &MorseSequencer::finished);

        QVERIFY(sequencer.play("THIS IS A LONG MESSAGE THAT WILL NOT BE PLAYED", MorseTiming::fromSpeed(20, 20, false)));
        QTest::qWait(300);
        sequencer.stop();
        QCOMPARE(finished.count(), 1);
        const qsizetype charsAtStop = chars.count();
        QTest::qWait(500);
        QCOMPARE(chars.count(), charsAtStop);
        QCOMPARE(finished.count(), 1);

        // A new playback right after a stop must work
        QVERIFY(sequencer.play("E", MorseTiming::fromSpeed(60, 60, false)));
        QVERIFY(finished.wait(3000));
        QCOMPARE(finished.count(), 2);
    }

    void refusesWithoutSoundOutput()
    {
        AudioEngine audio;
        MorseSequencer sequencer(&audio);
        QVERIFY(!sequencer.play("E", MorseTiming()));
    }
};

QTEST_GUILESS_MAIN(TestPlayback)
#include "tst_playback.moc"
