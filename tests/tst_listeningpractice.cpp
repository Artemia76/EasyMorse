// Tests for core/listeningpractice - part of EasyMorse, GPLv3
// The series is known by drawing it again from a course with the same seed.

#include <QTest>

#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>

#include "listeningpractice.h"
#include "morse/kochcourse.h"
#include "settings/settings.h"
#include "trainer.h"

namespace
{
constexpr std::uint64_t Seed = 1234;

///
/// \brief the series a practice with Seed plays first, without spaces
///
QString firstSeries(int pLevel)
{
    KochCourse course(Seed);
    course.setLevel(pLevel);
    QString text;
    for (int i = 0; i < ListeningPractice::GroupCount; ++i)
        text += course.nextGroup(ListeningPractice::GroupLength, true);
    return text;
}

void typeText(ListeningPractice& pPractice, const QString& pText)
{
    for (QChar character : pText)
        pPractice.type(QString(character));
}
}

class TestListeningPractice : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir   m_dir;
    int             m_count = 0;

    QString newSettingsFile() { return m_dir.filePath(QString("listening%1.ini").arg(++m_count)); }

private slots:
    void initTestCase()
    {
        qputenv("EASYMORSE_AUDIO_BACKEND", "null");
    }

    void startPlaysASeriesPrivately()
    {
        Settings settings(newSettingsFile());
        settings.setListeningKochLevel(3);
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        QCOMPARE(practice.kochLevel(), 3);
        QCOMPARE(practice.kochCharacters(), QString("KMU"));
        QSignalSpy shown(&trainer, &Trainer::charSent);

        QVERIFY(practice.start());
        QCOMPARE(practice.state(), ListeningPractice::Playing);
        QVERIFY(trainer.isPlaying());
        // Nothing tells the series before it is scored
        QVERIFY(practice.sent().isEmpty());
        QVERIFY(practice.results().isEmpty());
        QTest::qWait(600);
        QCOMPARE(shown.count(), 0);
        practice.stop();
    }

    void allRightPassesAndAddsACharacter()
    {
        Settings settings(newSettingsFile());
        settings.setListeningKochLevel(3);
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        QSignalSpy levelUp(&practice, &ListeningPractice::levelUp);

        const QString series = firstSeries(3);
        for (QChar character : series)
            QVERIFY(practice.kochCharacters().contains(character));
        QVERIFY(practice.start());
        typeText(practice, series.toLower());
        // The 25th character ends the series, and the sound
        QCOMPARE(practice.state(), ListeningPractice::Done);
        QVERIFY(!trainer.isPlaying());
        QCOMPARE(practice.sent().remove(' '), series);
        QCOMPARE(practice.sent().count(' '), ListeningPractice::GroupCount - 1);
        QCOMPARE(practice.total(), 25);
        QCOMPARE(practice.correct(), 25);
        QCOMPARE(practice.percent(), 100);
        QVERIFY(practice.isPassed());
        QCOMPARE(practice.kochLevel(), 4);
        QCOMPARE(settings.listeningKochLevel(), 4);
        QCOMPARE(levelUp.count(), 1);
        QCOMPARE(levelUp.at(0).at(0).toString(), QString("R"));
        // The keying practice keeps its own level
        QCOMPARE(settings.kochLevel(), Settings::MinKochLevel);
    }

    void ninetyPercentIsTheThreshold_data()
    {
        QTest::addColumn<int>("wrong");
        QTest::addColumn<bool>("passed");
        QTest::newRow("23 right") << 2 << true;
        QTest::newRow("22 right") << 3 << false;
    }

    void ninetyPercentIsTheThreshold()
    {
        QFETCH(int, wrong);
        QFETCH(bool, passed);
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);

        QString copy = firstSeries(Settings::MinKochLevel);
        for (int i = 0; i < wrong; ++i)
            copy[i * 7] = 'X';
        QVERIFY(practice.start());
        typeText(practice, copy);
        QCOMPARE(practice.state(), ListeningPractice::Done);
        QCOMPARE(practice.correct(), 25 - wrong);
        QCOMPARE(practice.isPassed(), passed);
        QCOMPARE(practice.kochLevel(), passed ? 3 : 2);
        const QVariantMap first = practice.results().at(0).toMap();
        QCOMPARE(first.value("status").toInt(), int(ListeningPractice::Wrong));
        QCOMPARE(first.value("copied").toString(), QString("X"));
        QCOMPARE(first.value("expected").toString(), QString(firstSeries(2).at(0)));
    }

    void typingKeepsMorseCharactersOnly()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        practice.type("K");         // not started: ignored
        QVERIFY(practice.typed().isEmpty());

        QVERIFY(practice.start());
        practice.type("k");
        practice.type(QString::fromUtf8("é"));
        practice.type("km");
        practice.type(" ");
        practice.type("?");
        QCOMPARE(practice.typed(), QString("K?"));
        practice.backspace();
        QCOMPARE(practice.typed(), QString("K"));
        practice.stop();
    }

    void endOfSoundLeavesTimeToCopy()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        QVERIFY(practice.start());
        practice.type("K");
        // As if the series was over
        trainer.stop();
        QCOMPARE(practice.state(), ListeningPractice::Finishing);
        practice.type("M");
        QCOMPARE(practice.typed(), QString("KM"));
        QTRY_COMPARE_WITH_TIMEOUT(practice.state(), ListeningPractice::Done,
                                  ListeningPractice::FinishDelayMs + 2000);
        QCOMPARE(practice.total(), 25);
        QVERIFY(!practice.isPassed());
        QCOMPARE(practice.kochLevel(), Settings::MinKochLevel);
        practice.finish();          // already scored: nothing changes
        QCOMPARE(practice.total(), 25);
    }

    void stopGivesUp()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        practice.setActive(true);
        QVERIFY(practice.start());
        practice.stop();
        QCOMPARE(practice.state(), ListeningPractice::Idle);
        QVERIFY(!trainer.isPlaying());
        QVERIFY(practice.results().isEmpty());
        practice.type("K");
        QVERIFY(practice.typed().isEmpty());

        // Leaving the page stops the series too
        QVERIFY(practice.start());
        practice.setActive(false);
        QCOMPARE(practice.state(), ListeningPractice::Idle);
        QVERIFY(!trainer.isPlaying());
        QTest::qWait(ListeningPractice::FinishDelayMs / 10);
        QCOMPARE(practice.state(), ListeningPractice::Idle);
    }

    void replayOnceScored()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        QSignalSpy shown(&trainer, &Trainer::charSent);
        QVERIFY(practice.start());
        practice.replay();          // not scored yet: nothing
        practice.finish();
        QCOMPARE(practice.state(), ListeningPractice::Done);
        QVERIFY(!trainer.isPlaying());
        practice.replay();
        QVERIFY(trainer.isPlaying());
        QCOMPARE(practice.state(), ListeningPractice::Done);
        practice.stop();
        QVERIFY(!trainer.isPlaying());
        QCOMPARE(shown.count(), 0);
        // A new series clears the result
        QVERIFY(practice.start());
        QCOMPARE(practice.state(), ListeningPractice::Playing);
        QVERIFY(practice.results().isEmpty());
        QVERIFY(practice.sent().isEmpty());
        practice.stop();
    }

    void newSeriesDuringReplay()
    {
        Settings settings(newSettingsFile());
        settings.setCharSpeed(Settings::MaxSpeed);
        settings.setWordSpeed(Settings::MaxSpeed);
        Trainer trainer(&settings);
        trainer.start();
        ListeningPractice practice(&trainer, Seed);
        practice.setActive(true);
        QVERIFY(practice.start());
        practice.finish();
        practice.replay();
        QVERIFY(trainer.isPlaying());
        // "New series" while the replay is still heard
        QVERIFY(practice.start());
        QCOMPARE(practice.state(), ListeningPractice::Playing);
        QVERIFY(trainer.isPlaying());
        QVERIFY(practice.sent().isEmpty());
        // Its end is not taken for the end of the new series
        QTest::qWait(ListeningPractice::BreakMs / 2);
        QCOMPARE(practice.state(), ListeningPractice::Playing);
        // The new series is played to its end, then left for the last characters
        QTRY_COMPARE_WITH_TIMEOUT(practice.state(), ListeningPractice::Finishing, 30000);
        practice.stop();
    }

    void noSoundNoSeries()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);     // not started: no sound output
        ListeningPractice practice(&trainer, Seed);
        QVERIFY(!practice.start());
        QCOMPARE(practice.state(), ListeningPractice::Idle);
    }
};

QTEST_GUILESS_MAIN(TestListeningPractice)
#include "tst_listeningpractice.moc"
