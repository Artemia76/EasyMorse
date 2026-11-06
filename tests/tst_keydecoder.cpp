// Tests for morse/keydecoder on simulated keying - part of EasyMorse, GPLv3

#include <QTest>

#include <QRandomGenerator>

#include "morse/keydecoder.h"
#include "morse/sequencer.h"

namespace
{
    struct Edge
    {
        bool    down;
        double  time;   // ms
    };

    ///
    /// \brief keying of a text, with optional random timing errors
    /// \param pJitter relative error applied to every element and gap (0.2 = +/-20 %)
    ///
    QList<Edge> keying(const QString& pText, const MorseTiming& pTiming, double pJitter = 0.0,
                       double pStartMs = 0.0, quint32 pSeed = 1)
    {
        QList<Edge> nominal;
        for (const auto& event : MorseSequencer::buildTimeline(pText, pTiming))
        {
            if (event.kind == TimelineEvent::Kind::KeyDown || event.kind == TimelineEvent::Kind::KeyUp)
                nominal.append({event.kind == TimelineEvent::Kind::KeyDown, event.time * 1000.0});
        }
        QRandomGenerator random(pSeed);
        QList<Edge> edges;
        double time = pStartMs;
        for (qsizetype i = 0; i < nominal.size(); ++i)
        {
            if (i > 0)
            {
                const double duration = nominal[i].time - nominal[i - 1].time;
                time += duration * (1.0 + pJitter * (2.0 * random.generateDouble() - 1.0));
            }
            edges.append({nominal[i].down, time});
        }
        return edges;
    }

    struct Result
    {
        QString         text;
        QList<double>   latencies;  // ms between the last key up of a character and its decoding
    };

    ///
    /// \brief feed edges to the decoder, advancing a simulated clock by 5 ms steps
    ///
    Result decode(KeyDecoder& pDecoder, const QList<Edge>& pEdges)
    {
        Result result;
        double lastUp = 0.0;
        double clock = 0.0;
        QObject context;
        QObject::connect(&pDecoder, &KeyDecoder::charDecoded, &context,
                         [&](const QString& text, const QString& pattern) {
            result.text += text.isEmpty() ? "[" + pattern + "]" : text;
            result.latencies.append(clock - lastUp);
        });
        QObject::connect(&pDecoder, &KeyDecoder::wordGap, &context, [&] { result.text += u' '; });

        qsizetype next = 0;
        const double end = pEdges.isEmpty() ? 0.0 : pEdges.last().time + 3000.0;
        for (clock = 0.0; clock <= end; clock += 5.0)
        {
            while (next < pEdges.size() && pEdges[next].time <= clock)
            {
                const double edgeTime = pEdges[next].time;
                const double saved = clock;
                clock = edgeTime;
                pDecoder.keyEdge(pEdges[next].down, qRound64(edgeTime));
                clock = saved;
                if (!pEdges[next].down)
                    lastUp = edgeTime;
                ++next;
            }
            pDecoder.advanceTo(qRound64(clock));
        }
        result.text = result.text.trimmed();
        return result;
    }

    std::unique_ptr<KeyDecoder> makeDecoder(double pWpm)
    {
        auto decoder = std::make_unique<KeyDecoder>();
        decoder->setAutoTimer(false);
        decoder->setSpeed(pWpm);
        return decoder;
    }

    ///
    /// \brief clumsy keying: random timing errors plus a badly timed element or
    /// silence every 6 to 8 elements (dot too long, dash too short, long pause)
    ///
    QList<Edge> clumsyKeying(const QString& pText, int pWpm, quint32 pSeed)
    {
        QList<Edge> edges = keying(pText, MorseTiming::fromSpeed(pWpm, pWpm, false), 0.2, 0.0, pSeed);
        QRandomGenerator random(pSeed + 1000);
        QList<Edge> result;
        double shift = 0.0;
        int next = 6 + random.bounded(3);
        for (qsizetype i = 0; i < edges.size(); ++i)
        {
            Edge edge = edges[i];
            edge.time += shift;
            if (!edge.down && i > 0 && --next == 0)
            {
                // Stretch or shrink this element by 40 to 70 %
                const double duration = edges[i].time - edges[i - 1].time;
                const double factor = random.bounded(2) ? 1.4 + 0.3 * random.generateDouble()
                                                        : 0.6 - 0.1 * random.generateDouble();
                const double change = duration * (factor - 1.0);
                edge.time += change;
                shift += change;
                next = 6 + random.bounded(3);
            }
            result.append(edge);
        }
        return result;
    }

    ///
    /// \brief largest relative error of the speed estimate after the first word
    ///
    double worstSpeedError(KeyDecoder::Reactivity pReactivity, int pWpm, quint32 pSeed)
    {
        auto decoder = makeDecoder(pWpm);
        decoder->setReactivity(pReactivity);
        const QList<Edge> edges = clumsyKeying("PARIS CQ CQ DE F4IKZ F4IKZ PSE K TNX FER QSO 73", pWpm, pSeed);
        const double warmUp = edges.size() / 8.0;
        double worst = 0.0;
        int count = 0;
        QObject context;
        QObject::connect(decoder.get(), &KeyDecoder::symbolProgress, &context, [&] {
            if (++count > warmUp)
                worst = qMax(worst, qAbs(decoder->speed() - pWpm) / pWpm);
        });
        decode(*decoder, edges);
        return worst;
    }
}

class TestKeyDecoder : public QObject
{
    Q_OBJECT

private:
    static void reactivities()
    {
        QTest::addColumn<KeyDecoder::Reactivity>("reactivity");
        QTest::newRow("fast") << KeyDecoder::Reactivity::Fast;
        QTest::newRow("normal") << KeyDecoder::Reactivity::Normal;
        QTest::newRow("stable") << KeyDecoder::Reactivity::Stable;
    }

private slots:
    void decodesAtNominalSpeed()
    {
        auto decoder = makeDecoder(20);
        const auto result = decode(*decoder, keying("PARIS PARIS", MorseTiming::fromSpeed(20, 20, false)));
        QCOMPARE(result.text, QString("PARIS PARIS"));
        QVERIFY(qAbs(decoder->speed() - 20.0) < 1.0);
    }

    void charactersComeOutWithoutWaitingForTheWord()
    {
        // Each character is decoded about 2 units after its last element,
        // well before the 3 units character gap is over
        auto decoder = makeDecoder(20);
        const auto result = decode(*decoder, keying("CQ DE F4IKZ", MorseTiming::fromSpeed(20, 20, false)));
        QCOMPARE(result.text, QString("CQ DE F4IKZ"));
        const double unit = 60.0;
        for (double latency : result.latencies)
            QVERIFY2(latency <= 3.0 * unit + 5.0, qPrintable(QString("latency %1 ms").arg(latency)));
    }

    void locksOnAFasterSender_data() { reactivities(); }
    void locksOnAFasterSender()
    {
        QFETCH(KeyDecoder::Reactivity, reactivity);
        auto decoder = makeDecoder(12);
        decoder->setReactivity(reactivity);
        const auto result = decode(*decoder, keying("PARIS PARIS PARIS", MorseTiming::fromSpeed(30, 30, false)));
        QCOMPARE(result.text, QString("PARIS PARIS PARIS"));
        QVERIFY2(qAbs(decoder->speed() - 30.0) < 2.0, qPrintable(QString::number(decoder->speed())));
    }

    void locksOnASlowerSender_data() { reactivities(); }
    void locksOnASlowerSender()
    {
        QFETCH(KeyDecoder::Reactivity, reactivity);
        auto decoder = makeDecoder(25);
        decoder->setReactivity(reactivity);
        const auto result = decode(*decoder, keying("PARIS PARIS PARIS", MorseTiming::fromSpeed(8, 8, false)));
        QVERIFY2(result.text.endsWith("PARIS PARIS"), qPrintable(result.text));
        QVERIFY2(qAbs(decoder->speed() - 8.0) < 1.0, qPrintable(QString::number(decoder->speed())));
    }

    void followsASpeedChangeInsideAMessage_data() { reactivities(); }
    void followsASpeedChangeInsideAMessage()
    {
        QFETCH(KeyDecoder::Reactivity, reactivity);
        auto decoder = makeDecoder(15);
        decoder->setReactivity(reactivity);
        QList<Edge> edges = keying("PARIS PARIS", MorseTiming::fromSpeed(15, 15, false));
        edges += keying("PARIS PARIS", MorseTiming::fromSpeed(25, 25, false), 0.0, edges.last().time + 1000.0);
        const auto result = decode(*decoder, edges);
        QVERIFY2(result.text.endsWith("PARIS PARIS"), qPrintable(result.text));
        QVERIFY2(qAbs(decoder->speed() - 25.0) < 2.0, qPrintable(QString::number(decoder->speed())));
    }

    void farnsworthSpacing()
    {
        // Character gaps are stretched far beyond 3 units: the word gap
        // threshold is learnt from the silences, in the first word or two
        auto decoder = makeDecoder(20);
        const auto result = decode(*decoder, keying("CQ CQ DE F4IKZ K", MorseTiming::fromSpeed(8, 20, true)));
        QVERIFY2(result.text.endsWith("CQ DE F4IKZ K"), qPrintable(result.text));
    }

    void toleratesHumanTiming_data()
    {
        QTest::addColumn<int>("wpm");
        QTest::addColumn<quint32>("seed");
        for (int wpm : {8, 15, 25, 35})
            for (quint32 seed = 1; seed <= 10; ++seed)
                QTest::newRow(qPrintable(QString("%1 wpm, seed %2").arg(wpm).arg(seed))) << wpm << seed;
    }

    void toleratesHumanTiming()
    {
        // +/-20 % on every element and gap, decoder starting at 15 WPM:
        // after the first call sign everything must be right
        QFETCH(int, wpm);
        QFETCH(quint32, seed);
        auto decoder = makeDecoder(15);
        const auto result = decode(*decoder, keying("CQ CQ DE F4IKZ F4IKZ K", MorseTiming::fromSpeed(wpm, wpm, false), 0.2, 0.0, seed));
        QVERIFY2(result.text.endsWith("CQ DE F4IKZ F4IKZ K"), qPrintable(result.text));
        QVERIFY2(qAbs(decoder->speed() - wpm) < 0.15 * wpm, qPrintable(QString::number(decoder->speed())));
    }

    void lockedSpeedWhenAutoAdaptIsOff()
    {
        auto decoder = makeDecoder(20);
        decoder->setAutoAdapt(false);
        decode(*decoder, keying("PARIS PARIS", MorseTiming::fromSpeed(15, 15, false)));
        QCOMPARE(decoder->speed(), 20.0);
    }

    void ignoresContactBounces()
    {
        auto decoder = makeDecoder(20);
        QList<Edge> edges = keying("EE", MorseTiming::fromSpeed(20, 20, false));
        // 3 ms bounce right after the first dot
        edges.insert(2, {true, edges[1].time + 20.0});
        edges.insert(3, {false, edges[1].time + 23.0});
        const auto result = decode(*decoder, edges);
        QCOMPARE(result.text, QString("EE"));
    }

    void clumsyKeyingBarelyMovesTheSpeed()
    {
        // Average over several keyings of the worst speed error after warm up
        double total[3] = {};
        constexpr int Runs = 10;
        for (quint32 seed = 1; seed <= Runs; ++seed)
            for (int r = 0; r < 3; ++r)
                total[r] += worstSpeedError(static_cast<KeyDecoder::Reactivity>(r), 15, seed);
        const double fast = total[0] / Runs, normal = total[1] / Runs, stable = total[2] / Runs;
        qInfo("worst speed error: fast %.1f %%, normal %.1f %%, stable %.1f %%",
              100 * fast, 100 * normal, 100 * stable);
        QVERIFY(normal < fast);
        QVERIFY(stable < normal);
    }

    void unknownPatternIsReported()
    {
        auto decoder = makeDecoder(20);
        QString text;
        QString pattern;
        connect(decoder.get(), &KeyDecoder::charDecoded, this, [&](const QString& t, const QString& p) {
            text = t;
            pattern = p;
        });
        QList<Edge> edges;
        for (int i = 0; i < 8; ++i)
        {
            edges.append({true, i * 120.0});
            edges.append({false, i * 120.0 + 60.0});
        }
        decode(*decoder, edges);
        QVERIFY(text.isEmpty());
        QCOMPARE(pattern, QString("........"));
    }

    void realTimeTimerEndsCharacters()
    {
        // With the internal timer, a character comes out without further key action
        KeyDecoder decoder;
        decoder.setSpeed(20);
        QString text;
        connect(&decoder, &KeyDecoder::charDecoded, this, [&](const QString& t, const QString&) { text += t; });
        const qint64 start = decoder.now();
        decoder.keyEdge(true, start);
        decoder.keyEdge(false, start + 60);
        QTRY_COMPARE_WITH_TIMEOUT(text, QString("E"), 1000);
    }
};

QTEST_GUILESS_MAIN(TestKeyDecoder)
#include "tst_keydecoder.moc"
