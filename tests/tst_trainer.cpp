// Tests for core/trainer on miniaudio's silent null device - part of EasyMorse, GPLv3

#include <QTest>

#include <QSignalSpy>
#include <QTemporaryDir>

#include "settings/settings.h"
#include "trainer.h"

class TestTrainer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir   m_dir;
    int             m_count = 0;

    QString newSettingsFile() { return m_dir.filePath(QString("trainer%1.ini").arg(++m_count)); }

private slots:
    void initTestCase()
    {
        qputenv("EASYMORSE_AUDIO_BACKEND", "null");
    }

    void keyIsDecodedInRealTime()
    {
        Settings settings(newSettingsFile());
        settings.setCharSpeed(20);
        Trainer trainer(&settings);
        trainer.start();
        QVERIFY(trainer.isAudioRunning());
        QSignalSpy decoded(&trainer, &Trainer::charDecoded);

        // "A" at 20 WPM: dot 60 ms, gap 60 ms, dash 180 ms
        trainer.manualKeyer()->press();
        QVERIFY(trainer.isKeyDown());
        QTest::qWait(60);
        trainer.manualKeyer()->release();
        QCOMPARE(trainer.decoderPattern(), QString("."));
        QTest::qWait(60);
        trainer.manualKeyer()->press();
        QTest::qWait(180);
        trainer.manualKeyer()->release();
        QVERIFY(!trainer.isKeyDown());

        QVERIFY(decoded.wait(1000));
        QCOMPARE(decoded.at(0).at(0).toString(), QString("A"));
        QVERIFY(trainer.decoderPattern().isEmpty());
    }

    void backgroundStationsAreNotDecoded()
    {
        Settings settings(newSettingsFile());
        settings.setBandPreset(Settings::Contest);
        Trainer trainer(&settings);
        trainer.start();
        QVERIFY(trainer.isAudioRunning());
        QSignalSpy decoded(&trainer, &Trainer::charDecoded);
        QSignalSpy finished(trainer.audio(), &AudioEngine::stationFinished);
        // The stations keep sending: each message that ends is followed by another
        QTRY_VERIFY_WITH_TIMEOUT(finished.count() >= 2, 20000);
        QCOMPARE(decoded.count(), 0);
        QVERIFY(!trainer.isKeyDown());
    }

    void keysAreCombined()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        ManualKeyer second;
        trainer.addKeyer(&second);
        QSignalSpy changes(&trainer, &Trainer::keyDownChanged);

        trainer.manualKeyer()->press();
        second.press();
        trainer.manualKeyer()->release();
        QVERIFY(trainer.isKeyDown());
        second.release();
        QVERIFY(!trainer.isKeyDown());
        QCOMPARE(changes.count(), 2);

        // A removed key held down is released
        second.press();
        QVERIFY(trainer.isKeyDown());
        trainer.removeKeyer(&second);
        QVERIFY(!trainer.isKeyDown());
    }

    void keysAreMutedWhilePlaying()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        QSignalSpy playing(&trainer, &Trainer::playingChanged);
        QVERIFY(trainer.play("EEEE"));
        QVERIFY(trainer.isPlaying());
        trainer.manualKeyer()->press();
        QVERIFY(!trainer.isKeyDown());
        trainer.manualKeyer()->release();
        trainer.stop();
        QVERIFY(!trainer.isPlaying());
        QCOMPARE(playing.count(), 2);
    }

    void deviceChangesRestartAudioOnce()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        QSignalSpy restarts(&trainer, &Trainer::audioRunningChanged);
        settings.setAudioDevice("No such card");
        settings.setAudioPeriodMs(20);
        QCOMPARE(restarts.count(), 0);
        QTRY_COMPARE(restarts.count(), 1);
        // Unknown card: falls back to the default output
        QVERIFY(trainer.isAudioRunning());
    }

    void lockedDecoderFollowsCharSpeed()
    {
        Settings settings(newSettingsFile());
        settings.setDecoderAutoAdapt(false);
        settings.setCharSpeed(25);
        Trainer trainer(&settings);
        QCOMPARE(qRound(trainer.decoderSpeed()), 25);
        settings.setCharSpeed(10);
        QCOMPARE(qRound(trainer.decoderSpeed()), 10);
        QVERIFY(!trainer.decoder()->autoAdapt());
        settings.setDecoderAutoAdapt(true);
        QVERIFY(trainer.decoder()->autoAdapt());
        settings.setDecoderReactivity(2);
        QCOMPARE(trainer.decoder()->reactivity(), KeyDecoder::Reactivity::Stable);
    }

    void remoteStationIsHeardAndDecoded()
    {
        Settings aliceSettings(newSettingsFile()), bobSettings(newSettingsFile());
        aliceSettings.setCallsign("f4ikz");
        aliceSettings.setStunServer(QString());
        bobSettings.setStunServer(QString());
        bobSettings.setJitterDelayMs(100);
        aliceSettings.setCharSpeed(20);
        bobSettings.setCharSpeed(20);
        Trainer alice(&aliceSettings), bob(&bobSettings);
        alice.start();
        bob.start();
        QCOMPARE(aliceSettings.callsign(), QString("F4IKZ"));

        alice.session()->start();
        bob.session()->start();
        QTRY_COMPARE_WITH_TIMEOUT(alice.session()->state(), PeerSession::State::Inviting, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.session()->state(), PeerSession::State::Inviting, 5000);
        QCOMPARE(alice.session()->join(bob.session()->invitation()), QString());
        QCOMPARE(bob.session()->join(alice.session()->invitation()), QString());
        QTRY_COMPARE_WITH_TIMEOUT(bob.session()->state(), PeerSession::State::Connected, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(alice.session()->state(), PeerSession::State::Connected, 10000);
        QCOMPARE(bob.session()->peerCallsign(), QString("F4IKZ"));

        QSignalSpy decoded(&bob, &Trainer::remoteCharDecoded);
        QSignalSpy remoteKey(&bob, &Trainer::remoteKeyDownChanged);
        // "N" at 20 WPM on Alice's key
        alice.manualKeyer()->press();
        QTest::qWait(180);
        alice.manualKeyer()->release();
        QTest::qWait(60);
        alice.manualKeyer()->press();
        QTest::qWait(60);
        alice.manualKeyer()->release();

        QTRY_VERIFY_WITH_TIMEOUT(bob.isRemoteActive(), 2000);
        QVERIFY(decoded.wait(2000));
        QCOMPARE(decoded.at(0).at(0).value<PeerSession*>(), bob.session());
        QCOMPARE(decoded.at(0).at(1).toString(), QString("N"));
        QCOMPARE(bob.remoteCallsign(), QString("F4IKZ"));
        QCOMPARE(remoteKey.count(), 4);
        // Bob's own decoder did not see Alice's key
        QVERIFY(bob.decoderPattern().isEmpty());
    }

    void playedTextReachesTheCorrespondent()
    {
        Settings aliceSettings(newSettingsFile()), bobSettings(newSettingsFile());
        aliceSettings.setCallsign("F4IKZ");
        for (Settings* settings : {&aliceSettings, &bobSettings})
        {
            settings->setStunServer(QString());
            settings->setCharSpeed(25);
            settings->setWordSpeed(25);
        }
        bobSettings.setJitterDelayMs(100);
        Trainer alice(&aliceSettings), bob(&bobSettings);
        alice.start();
        bob.start();
        alice.session()->start();
        bob.session()->start();
        QTRY_COMPARE_WITH_TIMEOUT(alice.session()->state(), PeerSession::State::Inviting, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(bob.session()->state(), PeerSession::State::Inviting, 5000);
        QCOMPARE(alice.session()->join(bob.session()->invitation()), QString());
        QCOMPARE(bob.session()->join(alice.session()->invitation()), QString());
        QTRY_COMPARE_WITH_TIMEOUT(bob.session()->state(), PeerSession::State::Connected, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(alice.session()->state(), PeerSession::State::Connected, 10000);

        QSignalSpy sent(&alice, &Trainer::charSent);
        QSignalSpy decoded(&bob, &Trainer::remoteCharDecoded);
        QVERIFY(alice.play("paris es"));
        QTRY_VERIFY_WITH_TIMEOUT(!alice.isPlaying(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(decoded.count(), 7, 5000);
        QString text, shown;
        for (const QList<QVariant>& arguments : std::as_const(decoded))
            text += arguments.at(1).toString();
        for (const QList<QVariant>& arguments : std::as_const(sent))
            shown += arguments.at(0).toString();
        // Played at its own rhythm, decoded on the other side
        QCOMPARE(text, QString("PARISES"));
        QCOMPARE(shown, QString("PARIS ES"));
        QVERIFY(!bob.isRemoteKeyDown());
        // The keys were not decoded locally
        QVERIFY(alice.decoderPattern().isEmpty());
    }

    void iambicPaddlesKeyAtTheSpeed()
    {
        Settings settings(newSettingsFile());
        settings.setCharSpeed(30);      // 40 ms dots
        settings.setDecoderAutoAdapt(false);
        Trainer trainer(&settings);
        trainer.start();
        QSignalSpy decoded(&trainer, &Trainer::charDecoded);

        // Straight mode: the paddles do nothing
        trainer.manualPaddles()->setDot(true);
        QTest::qWait(30);
        QVERIFY(!trainer.isKeyDown());
        trainer.manualPaddles()->release();

        settings.setKeyerMode(1);
        // The straight key is ignored in iambic mode
        trainer.manualKeyer()->press();
        QVERIFY(!trainer.isKeyDown());
        trainer.manualKeyer()->release();

        // "A": dot, then dash, from a quick squeeze. The dash paddle is released
        // as soon as the dash starts, whatever the load of the machine.
        int edges = 0;
        const auto releaseOnDash = connect(&trainer, &Trainer::keyDownChanged, this, [&] {
            if (++edges == 3)
                trainer.manualPaddles()->setDash(false);
        }, Qt::QueuedConnection);
        trainer.manualPaddles()->setDot(true);
        QVERIFY(trainer.isKeyDown());
        QVERIFY(trainer.isDotPaddle());
        trainer.manualPaddles()->setDash(true);
        trainer.manualPaddles()->setDot(false);
        QVERIFY(decoded.wait(2000));
        QCOMPARE(decoded.at(0).at(0).toString(), QString("A"));
        disconnect(releaseOnDash);

        // Left-handed: the dot paddle sends dashes
        settings.setPaddlesReversed(true);
        trainer.manualPaddles()->setDot(true);
        QVERIFY(trainer.isDashPaddle());
        trainer.manualPaddles()->setDot(false);
        QVERIFY(decoded.wait(2000));
        QCOMPARE(decoded.at(1).at(0).toString(), QString("T"));
    }

    void decoderEstimatesAreSaved()
    {
        const QString file = newSettingsFile();
        {
            Settings settings(file);
            Trainer trainer(&settings);
            trainer.decoder()->setEstimates(40.0, 120.0, 45.0);
        }
        Settings settings(file);
        Trainer trainer(&settings);
        QCOMPARE(trainer.decoder()->dotMs(), 40.0);
        QCOMPARE(qRound(trainer.decoderSpeed()), 30);
    }
};

QTEST_GUILESS_MAIN(TestTrainer)
#include "tst_trainer.moc"
