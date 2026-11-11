// Tests for core/settings - part of EasyMorse, GPLv3

#include <QTest>

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimeZone>

#include "settings/settings.h"

class TestSettings : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString file(const char* pName) const { return m_dir.filePath(pName); }

private slots:
    void defaults()
    {
        Settings settings(file("defaults.ini"));
        QCOMPARE(settings.toneFrequency(), 900);
        QCOMPARE(settings.volume(), 100);
        QCOMPARE(settings.wordSpeed(), 12);
        QCOMPARE(settings.charSpeed(), 18);
        QVERIFY(settings.farnsworth());
        QCOMPARE(settings.audioPeriodMs(), Settings::DefaultAudioPeriodMs);
        QVERIFY(settings.audioDevice().isEmpty());
        QVERIFY(!settings.serialEnabled());
        QVERIFY(settings.decoderAutoAdapt());
        QCOMPARE(settings.decoderReactivity(), 1);
        QVERIFY(settings.callsign().isEmpty());
        QCOMPARE(settings.stunServer(), QString(Settings::DefaultStunServer));
        QCOMPARE(settings.serverUrl(), QString(Settings::DefaultServerUrl));
        QCOMPARE(settings.jitterDelayMs(), 200);
        // Received Morse is not decoded by default: learn by ear
        QCOMPARE(settings.receiveDecodeMode(), 0);
        // Escape ends the capture of the mouse
        QCOMPARE(settings.releaseKey(), QString("%1:0").arg(int(Qt::Key_Escape)));
        // New versions are looked for unless the user says no
        QVERIFY(settings.checkUpdates());
        QVERIFY(settings.skippedVersion().isEmpty());
        QVERIFY(!settings.lastUpdateCheck().isValid());
        double dot, dash, gap;
        settings.decoderEstimates(dot, dash, gap);
        QCOMPARE(dot, 0.0);
    }

    void notifiesOnlyRealChanges()
    {
        Settings settings(file("notify.ini"));
        QSignalSpy spy(&settings, &Settings::wordSpeedChanged);
        settings.setWordSpeed(20);
        settings.setWordSpeed(20);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.wordSpeed(), 20);
    }

    void clampsValues()
    {
        Settings settings(file("clamp.ini"));
        settings.setToneFrequency(50);
        QCOMPARE(settings.toneFrequency(), Settings::MinFrequency);
        settings.setVolume(250);
        QCOMPARE(settings.volume(), 100);
        settings.setNoiseLevel(-1.0);
        QCOMPARE(settings.noiseLevel(), 0.0);
        settings.setBandwidth(10);
        QCOMPARE(settings.bandwidth(), Settings::MinBandwidth);
        settings.setQsbDepth(150);
        QCOMPARE(settings.qsbDepth(), 100);
        settings.setBreakInMs(-5);
        QCOMPARE(settings.breakInMs(), 0);
        settings.setBreakInMs(9000);
        QCOMPARE(settings.breakInMs(), Settings::MaxBreakInMs);
        settings.setCharSpeed(1000);
        QCOMPARE(settings.charSpeed(), Settings::MaxSpeed);
        settings.setDecoderReactivity(7);
        QCOMPARE(settings.decoderReactivity(), 2);
    }

    void persists()
    {
        {
            Settings settings(file("persist.ini"));
            settings.setToneFrequency(700);
            settings.setFarnsworth(false);
            settings.setAudioDevice("My card");
            settings.setSerialPort("ttyUSB0");
            settings.setDecoderEstimates(50.0, 150.0, 55.0);
            settings.setReleaseKey(QString("%1:2").arg(int(Qt::Key_Alt)));
            settings.setCheckUpdates(false);
            settings.setSkippedVersion("2.3.0");
            settings.setLastUpdateCheck(QDateTime(QDate(2026, 11, 11), QTime(10, 0), QTimeZone::UTC));
        }
        Settings settings(file("persist.ini"));
        QCOMPARE(settings.toneFrequency(), 700);
        QVERIFY(!settings.farnsworth());
        QCOMPARE(settings.audioDevice(), QString("My card"));
        QCOMPARE(settings.serialPort(), QString("ttyUSB0"));
        QCOMPARE(settings.releaseKey(), QString("%1:2").arg(int(Qt::Key_Alt)));
        QVERIFY(!settings.checkUpdates());
        QCOMPARE(settings.skippedVersion(), QString("2.3.0"));
        QCOMPARE(settings.lastUpdateCheck(), QDateTime(QDate(2026, 11, 11), QTime(10, 0), QTimeZone::UTC));
        double dot, dash, gap;
        settings.decoderEstimates(dot, dash, gap);
        QCOMPARE(dot, 50.0);
        QCOMPARE(dash, 150.0);
        QCOMPARE(gap, 55.0);
    }

    void bandPresets()
    {
        Settings settings(file("band.ini"));
        QCOMPARE(settings.bandPreset(), int(Settings::Clean));
        QCOMPARE(settings.noiseLevel(), 0.0);
        QCOMPARE(settings.breakInMs(), 500);
        QSignalSpy changed(&settings, &Settings::bandChanged);

        settings.setBandPreset(Settings::Contest);
        QCOMPARE(changed.count(), 1);
        // The transceiver is not part of the band conditions
        settings.setBreakInMs(200);
        QCOMPARE(settings.bandPreset(), int(Settings::Contest));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(settings.bandPreset(), int(Settings::Contest));
        QVERIFY(settings.contestStations() > 0);
        QVERIFY(settings.noiseLevel() > 0.0);
        QVERIFY(settings.agc());

        // A value changed by hand: custom conditions, kept as they are
        const int stations = settings.contestStations();
        settings.setQrnLevel(80);
        QCOMPARE(settings.bandPreset(), int(Settings::Custom));
        QCOMPARE(settings.contestStations(), stations);
        settings.setBandPreset(Settings::Custom);
        QCOMPARE(settings.qrnLevel(), 80);

        settings.setBandPreset(Settings::Aurora);
        QVERIFY(settings.flutter() > 0);
        QCOMPARE(settings.contestStations(), 0);
        settings.setBandPreset(Settings::Clean);
        QCOMPARE(settings.noiseLevel(), 0.0);
        QCOMPARE(settings.flutter(), 0);
        QCOMPARE(settings.qsbDepth(), 0);

        settings.setBandPreset(Settings::Urban);
        // A preset is applied again on reading: values out of step are fixed
        QSettings(file("band.ini"), QSettings::IniFormat).setValue("Band/ManMade", 3);
        Settings reread(file("band.ini"));
        QCOMPARE(reread.bandPreset(), int(Settings::Urban));
        QCOMPARE(reread.manMade(), settings.manMade());
    }

    void readsSettingsOfPreviousVersions()
    {
        {
            QSettings legacy(file("legacy.ini"), QSettings::IniFormat);
            legacy.setValue("Synth/SoundFreq", 650);
            legacy.setValue("Synth/NoiseCorrelation", 0.25);
            legacy.setValue("Morse/CharSpeed", 22);
            legacy.setValue("serial/PortEnable", true);
        }
        Settings settings(file("legacy.ini"));
        QCOMPARE(settings.toneFrequency(), 650);
        QCOMPARE(settings.noiseLevel(), 0.25);
        // The noise chosen then is kept: custom band conditions
        QCOMPARE(settings.bandPreset(), int(Settings::Custom));
        QCOMPARE(settings.charSpeed(), 22);
        QVERIFY(settings.serialEnabled());
    }
};

QTEST_GUILESS_MAIN(TestSettings)
#include "tst_settings.moc"
