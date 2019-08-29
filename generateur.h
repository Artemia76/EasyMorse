#ifndef GENERATEUR_H
#define GENERATEUR_H

#include <QAudioOutput>
#include <QByteArray>
#include <QIODevice>
#include <QObject>
#include <QMap>

class Generateur : public QIODevice
{
public:
    Generateur();

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    void setFormat(const QAudioFormat &pFormat);
    QAudioFormat getFormat();
    void setFrequency(int pFreq);
    int getFrequency();
    void setLoop(bool pLoop);
    void generateData(qint64 durationUs=1000000,bool pErase=false, bool pSilent=false);
    void clear();
private:
    qint64 m_pos = 0;
    bool m_loop;
    QByteArray m_buffer;
    QAudioFormat m_Format;
    int m_Freq;
};

#endif // GENERATEUR_H
