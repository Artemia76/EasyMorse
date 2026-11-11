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
