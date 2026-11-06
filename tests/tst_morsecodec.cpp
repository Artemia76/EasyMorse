// Tests for morse/morsecodec - part of EasyMorse, GPLv3

#include <QTest>

#include <QSet>

#include "morse/morsecodec.h"

class TestMorseCodec : public QObject
{
    Q_OBJECT

private slots:
    void encode_data()
    {
        QTest::addColumn<QChar>("character");
        QTest::addColumn<QString>("pattern");
        QTest::newRow("A") << QChar(u'A') << ".-";
        QTest::newRow("lowercase") << QChar(u'q') << "--.-";
        QTest::newRow("digit") << QChar(u'0') << "-----";
        QTest::newRow("question") << QChar(u'?') << "..--..";
        QTest::newRow("accent") << QChar(u'é') << "..-..";
        QTest::newRow("unknown") << QChar(u'#') << QString();
        QTest::newRow("space") << QChar(u' ') << QString();
    }

    void encode()
    {
        QFETCH(QChar, character);
        QFETCH(QString, pattern);
        QCOMPARE(MorseCodec::encode(character), pattern);
    }

    void decodeUnknown()
    {
        QVERIFY(MorseCodec::decode("........").isEmpty());
        QVERIFY(MorseCodec::decode(QString()).isEmpty());
    }

    void roundTrip()
    {
        for (const auto& entry : MorseCodec::table())
            QCOMPARE(MorseCodec::decode(MorseCodec::encode(entry.first)), QString(entry.first));
    }

    void patternsAreValidAndUnique()
    {
        QSet<QString> seen;
        for (const auto& entry : MorseCodec::table())
        {
            QVERIFY2(!entry.second.isEmpty(), qPrintable(QString(entry.first)));
            for (QChar symbol : entry.second)
                QVERIFY(symbol == u'.' || symbol == u'-');
            QVERIFY2(!seen.contains(entry.second), qPrintable("duplicate pattern " + entry.second));
            seen.insert(entry.second);
        }
    }
};

QTEST_GUILESS_MAIN(TestMorseCodec)
#include "tst_morsecodec.moc"
