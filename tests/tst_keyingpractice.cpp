// Tests for core/keyingpractice - part of EasyMorse, GPLv3
// The key edges and decoded characters are sent straight from the Trainer
// signals: no real time keying.

#include <QTest>

#include <QSignalSpy>
#include <QTemporaryDir>

#include "keyingpractice.h"
#include "morse/kochcourse.h"
#include "morse/morsecodec.h"
#include "settings/settings.h"
#include "trainer.h"

namespace
{
    ///
    /// \brief keys pText as a perfect operator (dot pUnitMs), from pTime
    ///
    void keyText(Trainer& pTrainer, const QString& pText, qint64& pTime, double pUnitMs = 100.0, double pDash = 3.0)
    {
        for (QChar character : pText)
        {
            const QString pattern = MorseCodec::encode(character);
            for (qsizetype i = 0; i < pattern.size(); ++i)
            {
                emit pTrainer.localKeyEdge(true, pTime);
                pTime += qRound64(pUnitMs * (pattern[i] == u'-' ? pDash : 1.0));
                emit pTrainer.localKeyEdge(false, pTime);
                pTime += qRound64(pUnitMs * (i + 1 < pattern.size() ? 1.0 : 3.0));
            }
            emit pTrainer.charDecoded(QString(character), pattern);
        }
    }
}

class TestKeyingPractice : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir   m_dir;
    int             m_count = 0;

    QString newSettingsFile() { return m_dir.filePath(QString("practice%1.ini").arg(++m_count)); }

private slots:
    void initTestCase()
    {
        qputenv("EASYMORSE_AUDIO_BACKEND", "null");
    }

    void groupFromTheLearntCharacters()
    {
        Settings settings(newSettingsFile());
        settings.setKochLevel(4);
        Trainer trainer(&settings);
        KeyingPractice practice(&trainer);
        QCOMPARE(practice.kochLevel(), 4);
        QCOMPARE(practice.kochCharacters(), QString("KMUR"));
        QVERIFY(practice.group().isEmpty());

        practice.setActive(true);
        QCOMPARE(practice.group().size(), KeyingPractice::GroupLength);
        for (QChar character : practice.group())
            QVERIFY(practice.kochCharacters().contains(character));

        // A new level draws a new group from the new characters
        practice.setKochLevel(2);
        QCOMPARE(settings.kochLevel(), 2);
        for (QChar character : practice.group())
            QVERIFY(QString("KM").contains(character));
    }

    void inactiveIgnoresTheKey()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        KeyingPractice practice(&trainer);
        practice.setActive(true);
        practice.setActive(false);
        qint64 time = 1000;
        keyText(trainer, practice.group(), time);
        QVERIFY(!practice.isLive());
        QVERIFY(!practice.isScored());
        QCOMPARE(practice.position(), 0);
    }

    void groupIsKeyedAndScored()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        KeyingPractice practice(&trainer);
        practice.setActive(true);
        const QString group = practice.group();

        qint64 time = 1000;
        emit trainer.localKeyEdge(true, time);
        QVERIFY(practice.isLive());
        QCOMPARE(practice.downSince(), 0.0);
        emit trainer.localKeyEdge(false, time += 300);
        QCOMPARE(practice.realSegments().size(), 1);
        practice.restart();         // start again from the first character
        QVERIFY(!practice.isLive());

        keyText(trainer, group.left(2), time);
        QVERIFY(practice.isLive());
        QCOMPARE(practice.position(), 2);
        QCOMPARE(practice.characters().size(), 2);
        QVERIFY(!practice.isScored());

        keyText(trainer, group.mid(2), time);
        QVERIFY(!practice.isLive());
        QVERIFY(practice.isScored());
        QCOMPARE(practice.analysisGroup(), group);
        QCOMPARE(practice.score(), 100);
        QCOMPARE(practice.verdict(), KeyingPractice::Excellent);
        QCOMPARE(practice.advice().size(), 1);
        QVERIFY(!practice.idealSegments().isEmpty());
        QVERIFY(qAbs(practice.unitMs() - 100.0) < 1.0);
        // The next group waits, the result stays
        QCOMPARE(practice.position(), 0);
        QCOMPARE(practice.group().size(), KeyingPractice::GroupLength);
        practice.restart();
        QVERIFY(practice.isScored());
    }

    void poorKeyingIsJudged()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        KeyingPractice practice(&trainer);
        practice.setActive(true);
        qint64 time = 1000;
        keyText(trainer, practice.group(), time, 100.0, 1.8);
        QVERIFY(practice.isScored());
        QVERIFY(practice.verdict() != KeyingPractice::Excellent);
        QVERIFY(practice.advice().first().contains("1"));
    }

    void goodGroupsAddACharacter()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        KeyingPractice practice(&trainer);
        QSignalSpy levelUp(&practice, &KeyingPractice::levelUp);
        practice.setActive(true);
        qint64 time = 1000;
        for (int i = 0; i < KochCourse::GroupsToPass; ++i)
        {
            keyText(trainer, practice.group(), time);
            time += 2000;
        }
        QCOMPARE(levelUp.count(), 1);
        QCOMPARE(levelUp.at(0).at(0).toString(), QString("U"));
        QCOMPARE(practice.kochLevel(), 3);
        QCOMPARE(settings.kochLevel(), 3);
    }

    void trainerReportsLocalEdges()
    {
        Settings settings(newSettingsFile());
        Trainer trainer(&settings);
        QSignalSpy edges(&trainer, &Trainer::localKeyEdge);
        trainer.manualKeyer()->press();
        trainer.manualKeyer()->release();
        QCOMPARE(edges.count(), 2);
        QCOMPARE(edges.at(0).at(0).toBool(), true);
        QCOMPARE(edges.at(1).at(0).toBool(), false);
    }
};

QTEST_GUILESS_MAIN(TestKeyingPractice)
#include "tst_keyingpractice.moc"
