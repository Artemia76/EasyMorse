// Tests for core/net jitter buffer on a simulated network - part of EasyMorse, GPLv3

#include <QTest>

#include <QRandomGenerator>

#include "net/jitterbuffer.h"

using Protocol::KeyEvent;

class TestJitterBuffer : public QObject
{
    Q_OBJECT

private:
    struct Played
    {
        bool    down;
        qint64  time;
    };

    ///
    /// \brief sends key events one per packet with the last ones repeated,
    /// over a network with a base delay, random jitter and lost packets
    ///
    static QList<Played> transmit(const QList<KeyEvent>& pEvents, int pDelayMs, int pJitterMs,
                                  double pLossRate, quint32 pSeed, int pBufferMs = 200)
    {
        JitterBuffer buffer;
        buffer.setAutoTimer(false);
        buffer.setDelay(pBufferMs);
        QList<Played> played;
        QObject::connect(&buffer, &JitterBuffer::keyChanged, &buffer, [&](bool down, qint64 time) {
            played.append({down, time});
        });

        // Remote clock is 100 s ahead of ours
        constexpr qint64 ClockDifference = 100000;
        QRandomGenerator random(pSeed);
        QMultiMap<qint64, QList<KeyEvent>> arrivals;
        for (qsizetype i = 0; i < pEvents.size(); ++i)
        {
            if (random.generateDouble() < pLossRate)
                continue;
            const qint64 sent = pEvents[i].timeMs - ClockDifference;
            const qint64 arrival = sent + pDelayMs + random.bounded(pJitterMs + 1);
            arrivals.insert(arrival, pEvents.mid(qMax<qsizetype>(0, i - 7), qMin<qsizetype>(i + 1, 8)));
        }
        qint64 now = arrivals.isEmpty() ? 0 : arrivals.firstKey();
        const qint64 end = pEvents.last().timeMs - ClockDifference + pDelayMs + pJitterMs + pBufferMs + 2000;
        auto next = arrivals.cbegin();
        for (; now <= end; ++now)
        {
            while (next != arrivals.cend() && next.key() <= now)
            {
                buffer.push(next.value(), now);
                ++next;
            }
            buffer.advanceTo(now);
        }
        return played;
    }

    static QList<KeyEvent> keying(int pCount, quint32 pStart = 0)
    {
        // Dots and dashes of 60 / 180 ms with 60 ms gaps
        QList<KeyEvent> events;
        quint32 time = 100000 + pStart;
        for (int i = 0; i < pCount; ++i)
        {
            events.append({static_cast<quint32>(2 * i), time, true});
            time += (i % 3 == 0) ? 180 : 60;
            events.append({static_cast<quint32>(2 * i + 1), time, false});
            time += 60;
        }
        return events;
    }

private slots:
    void keepsTheRhythmDespiteJitter()
    {
        const QList<KeyEvent> sent = keying(30);
        const QList<Played> played = transmit(sent, 40, 80, 0.0, 1);
        QCOMPARE(played.size(), sent.size());
        for (qsizetype i = 1; i < played.size(); ++i)
        {
            QCOMPARE(played[i].down, sent[i].down);
            const qint64 original = sent[i].timeMs - sent[i - 1].timeMs;
            const qint64 replayed = played[i].time - played[i - 1].time;
            // Jitter (80 ms) is below the buffer delay: elements are exact,
            // silences may absorb a small clock offset correction
            const qint64 tolerance = sent[i].down ? 11 : 1;
            QVERIFY2(qAbs(replayed - original) <= tolerance,
                     qPrintable(QString("event %1: %2 vs %3 ms").arg(i).arg(replayed).arg(original)));
        }
    }

    void recoversLostPacketsFromRedundancy()
    {
        const QList<KeyEvent> sent = keying(40);
        const QList<Played> played = transmit(sent, 40, 30, 0.3, 2);
        QCOMPARE(played.size(), sent.size());
        for (qsizetype i = 0; i < played.size(); ++i)
            QCOMPARE(played[i].down, sent[i].down);
    }

    void neverPlaysOutOfOrder()
    {
        // Jitter larger than the buffer: some events come late but keep their order
        const QList<KeyEvent> sent = keying(30);
        const QList<Played> played = transmit(sent, 40, 400, 0.1, 3, 100);
        QVERIFY(!played.isEmpty());
        for (qsizetype i = 1; i < played.size(); ++i)
        {
            QVERIFY(played[i].time >= played[i - 1].time);
            QVERIFY(played[i].down != played[i - 1].down || i == 0);
        }
    }

    void ignoresDuplicates()
    {
        JitterBuffer buffer;
        buffer.setAutoTimer(false);
        int count = 0;
        connect(&buffer, &JitterBuffer::keyChanged, this, [&] { ++count; });
        const QList<KeyEvent> events = {{5, 1000, true}, {6, 1060, false}};
        buffer.push(events, 2000);
        buffer.push(events, 2010);
        buffer.advanceTo(5000);
        buffer.push(events, 5000);
        buffer.advanceTo(9000);
        QCOMPARE(count, 2);
    }
};

QTEST_GUILESS_MAIN(TestJitterBuffer)
#include "tst_jitterbuffer.moc"
