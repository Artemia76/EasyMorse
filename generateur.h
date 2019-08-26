#ifndef GENERATEUR_H
#define GENERATEUR_H

#include <QAudioOutput>
#include <QByteArray>
#include <QIODevice>
#include <QObject>

class Generateur : public QIODevice
{
public:
    Generateur(const QAudioFormat &format, qint64 durationUs, int sampleRate);

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;

    private:
        void generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate);

    private:
        qint64 m_pos = 0;
        QByteArray m_buffer;
};

#endif // GENERATEUR_H
