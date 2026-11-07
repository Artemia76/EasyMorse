// Tests for core/settings - part of EasyMorse, GPLv3

#include <QTest>

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

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
        }
        Settings settings(file("persist.ini"));
        QCOMPARE(settings.toneFrequency(), 700);
        QVERIFY(!settings.farnsworth());
        QCOMPARE(settings.audioDevice(), QString("My card"));
        QCOMPARE(settings.serialPort(), QString("ttyUSB0"));
        double dot, dash, gap;
        settings.decoderEstimates(dot, dash, gap);
        QCOMPARE(dot, 50.0);
        QCOMPARE(dash, 150.0);
        QCOMPARE(gap, 55.0);
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
        QCOMPARE(settings.charSpeed(), 22);
        QVERIFY(settings.serialEnabled());
    }
};

QTEST_GUILESS_MAIN(TestSettings)
#include "tst_settings.moc"
