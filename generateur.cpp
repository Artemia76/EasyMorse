#include "generateur.h"

#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QDebug>
#include <QVBoxLayout>
#include <qmath.h>
#include <qendian.h>

Generateur::Generateur()
{
    m_Format.setSampleRate(44100);
    m_Format.setChannelCount(1);
    m_Format.setSampleSize(16);
    m_Format.setCodec("audio/pcm");
    m_Format.setByteOrder(QAudioFormat::LittleEndian);
    m_Format.setSampleType(QAudioFormat::SignedInt);
    m_Freq = 880;
    m_loop = true;
}

void Generateur::start()
{
    open(QIODevice::ReadOnly);
}

void Generateur::stop()
{
    m_pos = 0;
    close();
}
void Generateur::setFormat(const QAudioFormat &pFormat)
{
    if (pFormat.isValid())
        m_Format = pFormat;
}

QAudioFormat Generateur::getFormat()
{
    return m_Format;
}

void Generateur::setFrequency(int pFreq)
{
    m_Freq = pFreq;
}

int Generateur::getFrequency()
{
    return m_Freq;
}
void Generateur::setLoop(bool pLoop)
{
    m_loop = pLoop;
}

void Generateur::generateData(qint64 durationUs, bool pErase, bool pSilent)
{
    if (isOpen()) return;
    if (pErase) m_buffer.clear();
    const int channelBytes = m_Format.sampleSize() / 8;
    const int sampleBytes = m_Format.channelCount() * channelBytes;
    qint64 length = (m_Format.sampleRate() * m_Format.channelCount() * (m_Format.sampleSize() / 8))
                        * durationUs / 1000000;
    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes) // suppress warning in release builds
    int offset = m_buffer.size();
    m_buffer.resize(m_buffer.size() + static_cast<int>(length));
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data()+offset);
    int sampleIndex = 0;

    while (length) {
        // Produces value (-1..1)
        qreal x = 0.0;
        if (!pSilent) x = qSin(2 * M_PI * m_Freq * qreal(sampleIndex++ % m_Format.sampleRate()) / m_Format.sampleRate());
        for (int i=0; i<m_Format.channelCount(); ++i) {
            if (m_Format.sampleSize() == 8) {
                if (m_Format.sampleType() == QAudioFormat::UnSignedInt) {
                    const quint8 value = static_cast<quint8>((1.0 + x) / 2 * 255);
                    *reinterpret_cast<quint8 *>(ptr) = value;
                } else if (m_Format.sampleType() == QAudioFormat::SignedInt) {
                    const qint8 value = static_cast<qint8>(x * 127);
                    *reinterpret_cast<qint8 *>(ptr) = value;
                }
            } else if (m_Format.sampleSize() == 16) {
                if (m_Format.sampleType() == QAudioFormat::UnSignedInt) {
                    quint16 value = static_cast<quint16>((1.0 + x) / 2 * 65535);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint16>(value, ptr);
                    else
                        qToBigEndian<quint16>(value, ptr);
                } else if (m_Format.sampleType() == QAudioFormat::SignedInt) {
                    qint16 value = static_cast<qint16>(x * 32767);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint16>(value, ptr);
                    else
                        qToBigEndian<qint16>(value, ptr);
                }
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
    }
}

void Generateur::clear()
{
    if (isOpen()) return;
    m_buffer.clear();
}

qint64 Generateur::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty())
    {
        if (m_loop)
        {
            while (len - total > 0)
            {
                const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
                memcpy(data + total, m_buffer.constData() + m_pos, chunk);
                m_pos = (m_pos + chunk) % m_buffer.size();
                total += chunk;
            }
        }
        else
        {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            if (chunk<=0)
            {
                return -1;
            }
            memcpy(data, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk);
            total = chunk;
        }
    }
    return total;
}

qint64 Generateur::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 Generateur::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}
