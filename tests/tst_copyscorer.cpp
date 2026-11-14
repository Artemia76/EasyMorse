// Tests for morse/copyscorer - part of EasyMorse, GPLv3

#include <QTest>

#include "morse/copyscorer.h"

using Status = CopyScorer::Status;

namespace
{
// One letter per item: '=' correct, 'x' wrong, '-' missed
QString statuses(const CopyScorer& pScore)
{
    QString text;
    for (const CopyScorer::Item& item : pScore.items())
        text += item.status == Status::Correct ? '=' : item.status == Status::Wrong ? 'x' : '-';
    return text;
}
}

class TestCopyScorer : public QObject
{
    Q_OBJECT

private slots:
    void allRight()
    {
        const CopyScorer score = CopyScorer::score("KMKMR MRKUK", "kmkmrmrkuk");
        QCOMPARE(score.total(), 10);
        QCOMPARE(score.correct(), 10);
        QCOMPARE(score.extra(), 0);
        QCOMPARE(score.accuracy(), 1.0);
        QCOMPARE(statuses(score), QString("=========="));
    }

    void wrongCharacter()
    {
        const CopyScorer score = CopyScorer::score("KMRUE", "KMSUE");
        QCOMPARE(statuses(score), QString("==x=="));
        QCOMPARE(score.items().at(2).expected, QChar('R'));
        QCOMPARE(score.items().at(2).copied, QChar('S'));
        QCOMPARE(score.correct(), 4);
    }

    void missedCharacterDoesNotShift()
    {
        const CopyScorer score = CopyScorer::score("KMRUE SNAPT", "KMUE SNAPT");
        QCOMPARE(statuses(score), QString("==-======="));
        QVERIFY(score.items().at(2).copied.isNull());
        QCOMPARE(score.correct(), 9);
        QCOMPARE(score.extra(), 0);
    }

    void extraCharacter()
    {
        const CopyScorer score = CopyScorer::score("KMRUE", "KMMRUE");
        QCOMPARE(statuses(score), QString("====="));
        QCOMPARE(score.extra(), 1);
        QCOMPARE(score.correct(), 5);
    }

    void nothingCopied()
    {
        const CopyScorer score = CopyScorer::score("KM KM", "");
        QCOMPARE(statuses(score), QString("----"));
        QCOMPARE(score.accuracy(), 0.0);
    }

    void nothingSent()
    {
        const CopyScorer score = CopyScorer::score("", "AB");
        QCOMPARE(score.total(), 0);
        QCOMPARE(score.extra(), 2);
        QCOMPARE(score.accuracy(), 0.0);
    }

    void spacesAndCaseIgnored()
    {
        QCOMPARE(CopyScorer::normalize(" k m\tr "), QString("KMR"));
        const CopyScorer score = CopyScorer::score("KMR UE", "k mru e");
        QCOMPARE(score.correct(), 5);
    }

    void ninetyPercentOfASeries()
    {
        const QString sent = "KMRUE SNAPT KMRUE SNAPT KMRUE";
        QString copied = CopyScorer::normalize(sent);
        copied.remove(3, 1);    // one missed
        copied[10] = 'X';       // one wrong (after the shift)
        const CopyScorer score = CopyScorer::score(sent, copied);
        QCOMPARE(score.total(), 25);
        QCOMPARE(score.correct(), 23);
        QVERIFY(score.accuracy() >= 0.9);
    }
};

QTEST_GUILESS_MAIN(TestCopyScorer)
#include "tst_copyscorer.moc"
