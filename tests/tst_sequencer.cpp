// Tests for morse/sequencer timeline - part of EasyMorse, GPLv3

#include <QTest>

#include "morse/sequencer.h"

using Kind = TimelineEvent::Kind;

class TestSequencer : public QObject
{
    Q_OBJECT

private:
    static double endTime(const QList<TimelineEvent>& pTimeline)
    {
        return pTimeline.isEmpty() ? -1.0 : pTimeline.last().time;
    }

    static QList<double> keyTimes(const QList<TimelineEvent>& pTimeline)
    {
        QList<double> times;
        for (const auto& event : pTimeline)
            if (event.kind == Kind::KeyDown || event.kind == Kind::KeyUp)
                times.append(event.time);
        return times;
    }

private slots:
    void standardTiming()
    {
        const MorseTiming timing = MorseTiming::fromSpeed(20, 20, false);
        QCOMPARE(timing.unit, 0.06);
        QCOMPARE(timing.charGap, 0.18);
        QCOMPARE(timing.wordGap, 0.42);
    }

    void parisLasts50Units()
    {
        // PARIS standard: "PARIS " is 50 units, i.e. 3 s at 20 WPM
        const auto timeline = MorseSequencer::buildTimeline("PARIS ", MorseTiming::fromSpeed(20, 20, false));
        QVERIFY(qAbs(endTime(timeline) - 3.0) < 1e-9);
        QCOMPARE(timeline.last().kind, Kind::End);
    }

    void farnsworthKeepsOverallSpeed()
    {
        // Characters at 20 WPM, overall 10 WPM: "PARIS " must last 60 / 10 = 6 s
        const MorseTiming timing = MorseTiming::fromSpeed(10, 20, true);
        QCOMPARE(timing.unit, 0.06);
        QVERIFY(timing.charGap > 3 * timing.unit);
        QVERIFY(qAbs(timing.wordGap / timing.charGap - 7.0 / 3.0) < 1e-9);
        const auto timeline = MorseSequencer::buildTimeline("PARIS ", timing);
        QVERIFY(qAbs(endTime(timeline) - 6.0) < 1e-9);
    }

    void farnsworthIgnoredWhenNotSlower()
    {
        const MorseTiming timing = MorseTiming::fromSpeed(25, 20, true);
        QCOMPARE(timing.unit, 1.2 / 25);
        QCOMPARE(timing.charGap, 3 * timing.unit);
    }

    void elementsOfA()
    {
        const auto timeline = MorseSequencer::buildTimeline("A", MorseTiming::fromSpeed(12, 12, false));
        const double u = 0.1;
        const QList<double> expected = {0.0, u, 2 * u, 5 * u};
        const QList<double> times = keyTimes(timeline);
        QCOMPARE(times.size(), expected.size());
        for (qsizetype i = 0; i < times.size(); ++i)
            QVERIFY2(qAbs(times[i] - expected[i]) < 1e-9, qPrintable(QString::number(i)));
        QCOMPARE(timeline.first().kind, Kind::CharStart);
        QCOMPARE(timeline.first().index, 0);
    }

    void wordGapBetweenWords()
    {
        const auto timeline = MorseSequencer::buildTimeline("E  E", MorseTiming::fromSpeed(12, 12, false));
        // E ends at 1 unit, then 7 units of word gap (several spaces count once)
        QList<TimelineEvent> starts;
        for (const auto& event : timeline)
            if (event.kind == Kind::CharStart)
                starts.append(event);
        QCOMPARE(starts.size(), 2);
        QCOMPARE(starts[1].index, 3);
        QVERIFY(qAbs(starts[1].time - 0.8) < 1e-9);
    }

    void unknownCharactersAndLeadingSpacesAreSkipped()
    {
        const auto timeline = MorseSequencer::buildTimeline("  #E", MorseTiming::fromSpeed(12, 12, false));
        QCOMPARE(timeline.first().kind, Kind::CharStart);
        QCOMPARE(timeline.first().index, 3);
        QCOMPARE(timeline.first().time, 0.0);
    }

    void nothingToPlay()
    {
        QVERIFY(MorseSequencer::buildTimeline("", MorseTiming()).isEmpty());
        QVERIFY(MorseSequencer::buildTimeline("  #", MorseTiming()).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSequencer)
#include "tst_sequencer.moc"
