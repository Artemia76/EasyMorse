// Tests for tools/spscqueue.h - part of EasyMorse, GPLv3

#include <QTest>

#include <thread>

#include "tools/spscqueue.h"

class TestSpscQueue : public QObject
{
    Q_OBJECT

private slots:
    void emptyQueue()
    {
        SpscQueue<int, 4> queue;
        QVERIFY(queue.empty());
        QVERIFY(queue.front() == nullptr);
        QVERIFY(!queue.pop().has_value());
    }

    void fifoOrderAndCapacity()
    {
        SpscQueue<int, 4> queue;
        for (int i = 0; i < 4; ++i)
            QVERIFY(queue.push(i));
        QVERIFY2(!queue.push(99), "a full queue must refuse items");
        QCOMPARE(*queue.front(), 0);
        for (int i = 0; i < 4; ++i)
            QCOMPARE(queue.pop().value(), i);
        QVERIFY(queue.empty());
    }

    void wrapsAround()
    {
        SpscQueue<int, 4> queue;
        for (int i = 0; i < 1000; ++i)
        {
            QVERIFY(queue.push(i));
            QCOMPARE(queue.pop().value(), i);
        }
    }

    void concurrentProducerConsumer()
    {
        constexpr int Count = 1'000'000;
        SpscQueue<int, 1024> queue;
        std::thread producer([&queue] {
            for (int i = 0; i < Count; ++i)
                while (!queue.push(i))
                    std::this_thread::yield();
        });
        int expected = 0;
        bool inOrder = true;
        while (expected < Count)
        {
            if (auto value = queue.pop())
            {
                inOrder = inOrder && (*value == expected);
                ++expected;
            }
        }
        producer.join();
        QVERIFY(inOrder);
        QVERIFY(queue.empty());
    }
};

QTEST_GUILESS_MAIN(TestSpscQueue)
#include "tst_spscqueue.moc"
