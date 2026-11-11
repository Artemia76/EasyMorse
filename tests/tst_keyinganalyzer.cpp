// Tests for morse/keyinganalyzer - part of EasyMorse, GPLv3

#include <QTest>

#include "morse/keyinganalyzer.h"
#include "morse/morsecodec.h"

namespace
{
    ///
    /// \brief Proportions of a simulated operator, in dots
    ///
    struct Style
    {
        double  unitMs = 100.0;
        double  dash = 3.0;
        double  signGap = 1.0;
        double  charGap = 3.0;
    };

    ///
    /// \brief keys pSent and tells the analyzer each character as the decoder would
    /// \param pDecoded characters reported (default: pSent)
    ///
    void keyGroup(KeyingAnalyzer& pAnalyzer, const QString& pSent, const Style& pStyle,
                  const QString& pDecoded = QString())
    {
        double time = 1000.0;
        for (qsizetype c = 0; c < pSent.size(); ++c)
        {
            const QString pattern = MorseCodec::encode(pSent[c]);
            for (qsizetype i = 0; i < pattern.size(); ++i)
            {
                pAnalyzer.keyEdge(true, qRound64(time));
                time += pStyle.unitMs * (pattern[i] == u'-' ? pStyle.dash : 1.0);
                pAnalyzer.keyEdge(false, qRound64(time));
                time += pStyle.unitMs * (i + 1 < pattern.size() ? pStyle.signGap : pStyle.charGap);
            }
            const QString decoded = pDecoded.isEmpty() ? QString(pSent[c]) : QString(pDecoded[c]);
            pAnalyzer.charDecoded(decoded, pattern);
        }
        pAnalyzer.evaluate();
    }

    bool hasAdvice(const KeyingAnalyzer& pAnalyzer, KeyingAnalyzer::Advice pKind)
    {
        for (const auto& advice : pAnalyzer.advice())
        {
            if (advice.kind == pKind)
                return true;
        }
        return false;
    }
}

class TestKeyingAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    void perfectKeying()
    {
        KeyingAnalyzer analyzer;
        analyzer.start("KMURE", false, 60.0);
        QVERIFY(!analyzer.isStarted());
        keyGroup(analyzer, "KMURE", {});
        QVERIFY(analyzer.isComplete());
        QCOMPARE(analyzer.correctCount(), 5);
        QVERIFY(qAbs(analyzer.unitMs() - 100.0) < 1.0);
        QCOMPARE(analyzer.score(), 100);
        QCOMPARE(analyzer.advice().size(), 1);
        QCOMPARE(analyzer.advice().first().kind, KeyingAnalyzer::Advice::WellDone);
        QVERIFY(qAbs(analyzer.dashDotRatio() - 3.0) < 0.05);

        // Real and ideal agree: same element count and durations
        int realElements = 0;
        for (const auto& segment : analyzer.realSegments())
        {
            if (segment.kind == KeyingAnalyzer::Kind::Dot || segment.kind == KeyingAnalyzer::Kind::Dash)
            {
                ++realElements;
                QVERIFY(segment.rated);
                QVERIFY(qAbs(segment.error) < 0.02);
            }
        }
        int idealElements = 0;
        for (const auto& segment : analyzer.idealSegments())
        {
            if (segment.kind == KeyingAnalyzer::Kind::Dot || segment.kind == KeyingAnalyzer::Kind::Dash)
                ++idealElements;
        }
        QCOMPARE(realElements, 3 + 2 + 3 + 3 + 1);
        QCOMPARE(idealElements, realElements);
        QCOMPARE(analyzer.realSegments().first().start, 0.0);
    }

    void speedIsNotJudged()
    {
        // The reference is the speed of the operator: slow or fast, same score
        for (double unit : {240.0, 48.0})
        {
            KeyingAnalyzer analyzer;
            analyzer.start("PARIS", false, 100.0);
            keyGroup(analyzer, "PARIS", {.unitMs = unit});
            QVERIFY2(analyzer.score() >= 98, qPrintable(QString::number(analyzer.score())));
            QVERIFY(qAbs(analyzer.unitMs() - unit) < 1.0);
        }
    }

    void shortDashes()
    {
        KeyingAnalyzer analyzer;
        analyzer.start("KMURE", false, 100.0);
        keyGroup(analyzer, "KMURE", {.dash = 2.0});
        QVERIFY(qAbs(analyzer.dashDotRatio() - 2.0) < 0.05);
        QCOMPARE(analyzer.advice().first().kind, KeyingAnalyzer::Advice::DashesShort);
        QVERIFY2(analyzer.score() < 85, qPrintable(QString::number(analyzer.score())));
    }

    void lettersTooClose()
    {
        KeyingAnalyzer analyzer;
        analyzer.start("SNAPT", false, 100.0);
        keyGroup(analyzer, "SNAPT", {.charGap = 1.6});
        QVERIFY(hasAdvice(analyzer, KeyingAnalyzer::Advice::LettersClose));
        bool closeGapRated = false;
        for (const auto& segment : analyzer.realSegments())
        {
            if (segment.kind == KeyingAnalyzer::Kind::CharGap)
            {
                QVERIFY(segment.rated);
                closeGapRated = closeGapRated || segment.error < -KeyingAnalyzer::FairError;
            }
        }
        QVERIFY(closeGapRated);
    }

    void slowSpacingIsTolerated()
    {
        // Farnsworth spacing: letters 5 dots apart is only slow
        KeyingAnalyzer analyzer;
        analyzer.start("SNAPT", false, 100.0);
        keyGroup(analyzer, "SNAPT", {.charGap = 5.0});
        QCOMPARE(analyzer.score(), 100);
    }

    void wrongCharacter()
    {
        KeyingAnalyzer analyzer;
        analyzer.start("KMURE", false, 100.0);
        // D keyed instead of M
        keyGroup(analyzer, "KDURE", {}, "KDURE");
        QCOMPARE(analyzer.correctCount(), 4);
        QVERIFY(!analyzer.characters().at(1).correct);
        QCOMPARE(analyzer.characters().at(1).expectedPattern, QString("--"));
        const auto& advice = analyzer.advice().first();
        QCOMPARE(advice.kind, KeyingAnalyzer::Advice::WrongCharacter);
        QCOMPARE(advice.text, QString("M"));
        QCOMPARE(advice.detail, QString("-.."));
        QVERIFY(analyzer.score() < 100);
    }

    void iambicJudgesTheSpacing()
    {
        // The keyer makes the elements: short dashes are not the operator's
        KeyingAnalyzer analyzer;
        analyzer.start("KMURE", true, 100.0);
        keyGroup(analyzer, "KMURE", {.dash = 2.0});
        QVERIFY(!hasAdvice(analyzer, KeyingAnalyzer::Advice::DashesShort));
        for (const auto& segment : analyzer.realSegments())
            QCOMPARE(segment.rated, segment.kind == KeyingAnalyzer::Kind::CharGap);

        KeyingAnalyzer close;
        close.start("KMURE", true, 100.0);
        keyGroup(close, "KMURE", {.charGap = 1.5});
        QVERIFY(hasAdvice(close, KeyingAnalyzer::Advice::LettersClose));
    }

    void bouncesAreIgnored()
    {
        KeyingAnalyzer analyzer;
        analyzer.start("EE", false, 100.0);
        analyzer.keyEdge(true, 0);
        analyzer.keyEdge(false, 100);
        analyzer.keyEdge(true, 150);    // 4 ms bounce inside the gap
        analyzer.keyEdge(false, 154);
        analyzer.charDecoded("E", ".");
        analyzer.keyEdge(true, 400);
        QCOMPARE(analyzer.downSince(), 400.0);
        analyzer.keyEdge(false, 500);
        QCOMPARE(analyzer.downSince(), -1.0);
        analyzer.charDecoded("E", ".");
        analyzer.evaluate();
        QCOMPARE(analyzer.realSegments().size(), 3);   // dot, gap, dot
        QCOMPARE(analyzer.realSegments().at(1).kind, KeyingAnalyzer::Kind::CharGap);
        QCOMPARE(analyzer.realSegments().at(1).end - analyzer.realSegments().at(1).start, 300.0);
    }

    void partialGroup()
    {
        // Evaluated while keying: the character not decoded yet is not rated
        KeyingAnalyzer analyzer;
        analyzer.start("KM", false, 100.0);
        analyzer.keyEdge(true, 0);
        analyzer.keyEdge(false, 300);
        analyzer.keyEdge(true, 400);
        analyzer.keyEdge(false, 500);
        analyzer.keyEdge(true, 600);
        analyzer.keyEdge(false, 900);
        analyzer.charDecoded("K", "-.-");
        analyzer.keyEdge(true, 1200);
        analyzer.keyEdge(false, 1500);
        analyzer.evaluate();
        QVERIFY(!analyzer.isComplete());
        const auto& last = analyzer.realSegments().last();
        QCOMPARE(last.charIndex, -1);
        QVERIFY(!last.rated);
        QCOMPARE(last.kind, KeyingAnalyzer::Kind::Dash);

        // Further characters past the group are ignored
        analyzer.keyEdge(true, 1600);
        analyzer.keyEdge(false, 1900);
        analyzer.charDecoded("M", "--");
        QVERIFY(analyzer.isComplete());
        const qsizetype segments = (analyzer.evaluate(), analyzer.realSegments().size());
        analyzer.keyEdge(true, 2500);
        analyzer.keyEdge(false, 2600);
        analyzer.charDecoded("E", ".");
        analyzer.evaluate();
        QCOMPARE(analyzer.realSegments().size(), segments);
        QCOMPARE(analyzer.characters().size(), 2);
    }
};

QTEST_GUILESS_MAIN(TestKeyingAnalyzer)
#include "tst_keyinganalyzer.moc"
