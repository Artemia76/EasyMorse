/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
** Contact: https://bitbucket.org/Artemia/easymorse/src/master/
**
** FFS2Play is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** FFS2Play is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** The license is as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

/****************************************************************************
** generator.cpp is part of EasyMorse project
**
** This class provide way to synthesize wave
** form signal to compose morse tone
****************************************************************************/

#include "generator.h"

#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QDebug>
#include <QVBoxLayout>
#include <qmath.h>
#include <qendian.h>
#include <math.h>

QAudioFormat CGenerator::m_Format;

CGenerator::CGenerator()
{

    m_Freq = 900;
    init();
}

CGenerator::CGenerator(int pFrequency)
{
    m_Freq = pFrequency;
    init();
}

void CGenerator::init()
{
    m_log = CLogger::instance();
    m_loop = true;
    m_buffer.clear();
    m_buffer.resize(0);
    m_noise_correlation=0.0;
    m_LastSample=0.0;
    m_Lowfilter_T=0;
}

void CGenerator::start()
{
    open(QIODevice::ReadOnly);
}

void CGenerator::stop()
{
    m_pos = 0;
    close();
}
void CGenerator::setFormat(const QAudioFormat &pFormat)
{
    if (pFormat.isValid())
    {
        m_Format = pFormat;
    }
}

QAudioFormat CGenerator::getFormat()
{
    return m_Format;
}

void CGenerator::setFrequency(int pFreq)
{
    m_Freq = pFreq;
}

int CGenerator::getFrequency()
{
    return m_Freq;
}
void CGenerator::setLoop(bool pLoop)
{
    m_loop = pLoop;
}

void CGenerator::setNoiseCorrelation(qreal pNoiseCorrelation)
{
    if ((pNoiseCorrelation>=0.0) && (pNoiseCorrelation<=1.0))
    {
        m_noise_correlation = pNoiseCorrelation;
    }
}
void CGenerator::setNoiseFilter(int pNoiseFilter)
{
    if ((pNoiseFilter>=0) && (pNoiseFilter<=2000))
    {
        m_Lowfilter_T = pNoiseFilter;
    }
}

void CGenerator::generateData(qint64 durationUs, bool pErase, bool pSilent)
{
    if (pErase)
    {
        m_buffer.clear();
        m_buffer.resize(0);
        m_pos=0;
    }
    const int channelBytes = m_Format.sampleSize() / 8;
    const int sampleBytes = m_Format.channelCount() * channelBytes;
    const int sampleRate = m_Format.sampleRate();
    quint64 length = (sampleRate * sampleBytes * durationUs) / 1000000;
    length -= (length % sampleBytes);
    int offset = m_buffer.size();
    m_buffer.resize(m_buffer.size() + static_cast<int>(length));
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data()+offset);
    int sampleIndex = 0;
    m_LastSample = 0.0;
    qreal TimeStep = -1.0 * static_cast<qreal>(durationUs)/static_cast<qreal>(length);
    while (length) {
        // Produces value (-1..1)
        qreal w = 0.0; //Wave
        qreal n = 0.0; //Noise
        qreal m = 0.0; // Mixing
        // Wave generator in function of Samplerate and frequency
        if (!pSilent)
        {
            w = qSin(2 * M_PI * m_Freq * qreal(sampleIndex++ % m_Format.sampleRate()) / m_Format.sampleRate())
                * (1.0-m_noise_correlation);
        }
        // Pure Noise generator
        if ((m_noise_correlation > 0.0) && (m_noise_correlation <=1.0))
        {
            n = ((m_Rnd.generateDouble()*2.0)-1.0)*m_noise_correlation;
        }
        //Applying filter
        if (m_Lowfilter_T > 0.0)
        {
            qreal Diff = n - m_LastSample;
            qreal Filter = (1- exp(TimeStep/m_Lowfilter_T));
            n = m_LastSample + (Diff * Filter);
            m_LastSample = n;
        }
        //Compressor to back nois at 100% level

        m = w+n;
        for (int i=0; i<m_Format.channelCount(); ++i)
        {
            if (m_Format.sampleSize() == 8)
            {
                if (m_Format.sampleType() == QAudioFormat::UnSignedInt)
                {
                    const quint8 value = static_cast<quint8>((1.0 + m) / 2 * 255);
                    *reinterpret_cast<quint8 *>(ptr) = value;
                }
                else if (m_Format.sampleType() == QAudioFormat::SignedInt)
                {
                    const qint8 value = static_cast<qint8>(m * 127);
                    *reinterpret_cast<qint8 *>(ptr) = value;
                }
            }
            else if (m_Format.sampleSize() == 16)
            {
                if (m_Format.sampleType() == QAudioFormat::UnSignedInt)
                {
                    quint16 value = static_cast<quint16>((1.0 + m) / 2 * 65535);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint16>(value, ptr);
                    else
                        qToBigEndian<quint16>(value, ptr);
                }
                else if (m_Format.sampleType() == QAudioFormat::SignedInt)
                {
                    qint16 value = static_cast<qint16>(m * 32767);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint16>(value, ptr);
                    else
                        qToBigEndian<qint16>(value, ptr);
                }
            }
            else if (m_Format.sampleSize() == 24)
            {
                if (m_Format.sampleType() == QAudioFormat::UnSignedInt)
                {
                    quint32 value = static_cast<quint32>((1.0 + m) / 2 * 16777215);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint32>(value, ptr);
                    else
                        qToBigEndian<quint32>(value, ptr);
                }
                else if (m_Format.sampleType() == QAudioFormat::SignedInt)
                {
                    qint32 value = static_cast<qint32>(m * 8388607);
                    if (m_Format.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint32>(value, ptr);
                    else
                        qToBigEndian<qint32>(value, ptr);
                }
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
    }
}

void CGenerator::clear()
{
    if (isOpen()) stop();
    m_buffer.clear();
    m_buffer.resize(0);
}

qint64 CGenerator::readData(char *data, qint64 len)
{
    if (!isOpen()) return -1;
    qint64 total = 0;
    if (!m_buffer.isEmpty())
    {
        if (m_loop)
        {
            while (len - total > 0)
            {
                const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
                memcpy(data + total, m_buffer.constData() + m_pos, static_cast<size_t>(chunk));
                m_pos = (m_pos + chunk) % m_buffer.size();
                total += chunk;
            }
        }
        else
        {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len);
            if (chunk>0)
            {
                memcpy(data, m_buffer.constData() + m_pos, static_cast<size_t>(chunk));
                total = chunk;
                m_pos = (m_pos + chunk);
            }
            else
            {
                total = 0;
            }
        }
    }
    return total;
}

qint64 CGenerator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 CGenerator::pos() const
{
    return m_pos;
}

qint64 CGenerator::bytesAvailable() const
{
    if (m_loop)
    {
        return m_buffer.size() + QIODevice::bytesAvailable();
    }
    else
    {
        return m_buffer.size()-m_pos;
    }
}

bool CGenerator::atEnd() const
{
    if ((!m_loop) && (m_pos >= m_buffer.size())) return true;
    else return false;
}
