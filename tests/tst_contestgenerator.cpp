// Tests of the background stations of a contest - part of EasyMorse, GPLv3
#include <QTest>

#include <QRegularExpression>
#include <QSet>

#include "morse/contestgenerator.h"
#include "morse/morsecodec.h"

namespace
{
    constexpr double Rate = 48000.0;

    // Key down durations of a script, in dot units
    QString pattern(const band::StationScript& pScript, double pDot)
    {
        QString out;
        for (int i = 1; i < pScript.count; i += 2)
            out += pScript.durations[i] > 2.0 * pDot ? '-' : '.';
        return out;
    }
}

class TestContestGenerator : public QObject
{
    Q_OBJECT

private slots:
    void callsignsLookReal()
    {
        ContestGenerator generator(Rate, 42);
        const QRegularExpression format("^[A-Z0-9]{1,3}[0-9][A-Z]{1,3}$");
        QSet<QString> calls;
        for (int i = 0; i < 500; ++i)
        {
            const QString call = generator.randomCallsign();
            QVERIFY2(format.match(call).hasMatch(), qPrintable(call));
            calls.insert(call);
        }
        QVERIFY(calls.size() > 450);
    }

    void stationsSendContestMessages()
    {
        ContestGenerator generator(Rate, 7);
        const QRegularExpression contest("^(CQ |TU |5NN |[A-Z0-9]+[0-9][A-Z]+)");
        int cq = 0, report = 0;
        for (int slot = 0; slot < band::BandSimulator::MaxStations; ++slot)
        {
            for (int i = 0; i < 40; ++i)
            {
                const band::StationScript script = generator.next(slot);
                const QString text = generator.lastText(slot);
                QVERIFY2(contest.match(text).hasMatch(), qPrintable(text));
                cq += text.startsWith("CQ") ? 1 : 0;
                report += text.contains("5NN") ? 1 : 0;

                QCOMPARE(script.slot, slot);
                QVERIFY(script.count >= 2 && script.count <= band::StationScript::MaxDurations);
                // Pause first, then key down, key up..., ending on a key down
                QCOMPARE(script.count % 2, 0);
                QVERIFY(script.durations[0] >= 0.3 * Rate * 0.9);
                for (int d = 1; d < script.count; ++d)
                    QVERIFY(script.durations[d] > 0);
                // Above or below the listening frequency, never on it
                QVERIFY(std::abs(script.offsetHz) >= 80.0f && std::abs(script.offsetHz) <= 1500.0f);
                QVERIFY(script.levelDb >= -18.0f && script.levelDb <= 6.0f);
            }
        }
        QVERIFY(cq > 20);
        QVERIFY(report > 20);
    }

    void scriptsAreValidMorse()
    {
        ContestGenerator generator(Rate, 3);
        for (int i = 0; i < 30; ++i)
        {
            const band::StationScript script = generator.next(0);
            const QString text = generator.lastText(0);
            // Elements in the order of the text
            QString expected;
            for (const QChar character : text)
                expected += MorseCodec::encode(character);
            // The shortest key down is a dot (up to the 6 % of a hand sent signal)
            double dot = 1e9;
            for (int d = 1; d < script.count; d += 2)
                dot = std::min(dot, double(script.durations[d]));
            QCOMPARE(pattern(script, dot), expected);
        }
    }
};

QTEST_GUILESS_MAIN(TestContestGenerator)
#include "tst_contestgenerator.moc"
