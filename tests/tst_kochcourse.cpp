// Tests for morse/kochcourse - part of EasyMorse, GPLv3

#include <QTest>

#include "morse/kochcourse.h"
#include "morse/morsecodec.h"

class TestKochCourse : public QObject
{
    Q_OBJECT

private slots:
    void orderHasEveryCharacterOnce()
    {
        const QString order = KochCourse::order();
        QCOMPARE(order.size(), KochCourse::maxLevel());
        for (QChar character : order)
        {
            QVERIFY2(!MorseCodec::encode(character).isEmpty(), qPrintable(QString(character)));
            QCOMPARE(order.count(character), 1);
        }
        QVERIFY(order.startsWith("KMURES"));
    }

    void levelIsBounded()
    {
        KochCourse course(1);
        course.setLevel(0);
        QCOMPARE(course.level(), KochCourse::MinLevel);
        course.setLevel(1000);
        QCOMPARE(course.level(), KochCourse::maxLevel());
        course.setLevel(4);
        QCOMPARE(course.characters(), QString("KMUR"));
        QCOMPARE(course.newest(), QChar('R'));
    }

    void groupsUseTheLearntCharacters()
    {
        KochCourse course(42);
        course.setLevel(5);
        int newest = 0;
        int total = 0;
        for (int i = 0; i < 400; ++i)
        {
            const QString group = course.nextGroup();
            QCOMPARE(group.size(), 5);
            for (qsizetype j = 0; j < group.size(); ++j)
            {
                QVERIFY(course.characters().contains(group[j]));
                if (j > 0)
                    QVERIFY(group[j] != group[j - 1]);
                newest += group[j] == course.newest() ? 1 : 0;
                ++total;
            }
        }
        // About a third, a bit less as it never comes twice in a row
        const double share = static_cast<double>(newest) / total;
        QVERIFY2(share > 0.25 && share < 0.45, qPrintable(QString::number(share)));
    }

    void copyGroupsMayRepeat()
    {
        // By ear, two characters must not simply alternate
        KochCourse course(42);
        course.setLevel(2);
        int repeats = 0;
        int pairs = 0;
        for (int i = 0; i < 400; ++i)
        {
            const QString group = course.nextGroup(5, true);
            QCOMPARE(group.size(), 5);
            for (qsizetype j = 1; j < group.size(); ++j)
            {
                QVERIFY(course.characters().contains(group[j]));
                repeats += group[j] == group[j - 1] ? 1 : 0;
                ++pairs;
            }
        }
        // Newest 2/3 of the time with two characters: repeats about 5/9
        const double share = static_cast<double>(repeats) / pairs;
        QVERIFY2(share > 0.45 && share < 0.65, qPrintable(QString::number(share)));
    }

    void focusComesMoreOften()
    {
        KochCourse course(42);
        course.setLevel(10);
        const QChar focus = u'U';
        auto share = [&course](QChar pFocus, bool pRepeats) {
            int hits = 0;
            int total = 0;
            for (int i = 0; i < 2000; ++i)
            {
                const QString group = course.nextGroup(5, pRepeats, pFocus);
                for (qsizetype j = 0; j < group.size(); ++j)
                {
                    if (!pRepeats && j > 0 && group[j] == group[j - 1])
                        return -1.0;
                    hits += group[j] == u'U' ? 1 : 0;
                    ++total;
                }
            }
            return static_cast<double>(hits) / total;
        };
        // 2/3 x (1/4 + 3/4 x 1/10) = 0.217, against 2/3 x 1/10 = 0.067 without focus
        const double focused = share(focus, true);
        QVERIFY2(focused > 0.19 && focused < 0.245, qPrintable(QString::number(focused)));
        const double plain = share(QChar(), true);
        QVERIFY2(plain > 0.05 && plain < 0.085, qPrintable(QString::number(plain)));
        // Never twice in a row for keying
        QVERIFY(share(focus, false) > 2 * plain);
        // A character not learnt is ignored
        for (int i = 0; i < 100; ++i)
            QVERIFY(!course.nextGroup(5, false, u'X').contains(u'X'));
    }

    void groupsAreReproducible()
    {
        KochCourse first(7);
        KochCourse second(7);
        first.setLevel(10);
        second.setLevel(10);
        for (int i = 0; i < 20; ++i)
            QCOMPARE(first.nextGroup(), second.nextGroup());
    }

    void goodGroupsAddACharacter()
    {
        KochCourse course(1);
        course.setLevel(2);
        for (int i = 0; i < KochCourse::GroupsToPass - 1; ++i)
            QVERIFY(!course.recordResult(5, 5, 90));
        QVERIFY(course.recordResult(5, 5, 90));
        QCOMPARE(course.level(), 3);

        // One bad group in the last five: not yet
        for (int i = 0; i < KochCourse::GroupsToPass - 1; ++i)
            QVERIFY(!course.recordResult(5, 5, 90));
        QVERIFY(!course.recordResult(3, 5, 90));
        QCOMPARE(course.level(), 3);
        // Right characters but poor keying: not yet either
        for (int i = 0; i < KochCourse::GroupsToPass; ++i)
            QVERIFY(!course.recordResult(5, 5, 50));
        QCOMPARE(course.level(), 3);
    }
};

QTEST_GUILESS_MAIN(TestKochCourse)
#include "tst_kochcourse.moc"
