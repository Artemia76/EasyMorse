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

#include <QMediaDevices>
#include <QAudioOutput>
#include <QDebug>
#include <QVBoxLayout>
#include <qmath.h>
#include <qendian.h>
#include <QSysInfo>
#include <math.h>

QAudioFormat CGenerator::m_Format;

CGenerator::CGenerator()
{

    m_Freq = 900;
    m_FadeTime = 1000;
    m_Amplitude =0.0;
    m_log = CLogger::instance();
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
    m_sample.clear();
    m_sample.resize(0);
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
        m_signal.clear();
        m_signal.resize(0);
        m_sample.clear();
        m_sample.resize(0);
        m_pos=0;
    }
    const int sampleRate = m_Format.sampleRate();
    quint64 length = sampleRate * static_cast<qreal>(durationUs) / static_cast<qreal>(1000000);
    int sampleIndex = 0;
    while (length)
    {
        qreal w = 0.0; //Wave
        // Wave generator in function of Samplerate and frequency
        if (!pSilent)
        {
            w = qSin(2.0 * M_PI * static_cast<qreal>(m_Freq) * static_cast<qreal>(sampleIndex++ % m_Format.sampleRate()) / static_cast<qreal>(m_Format.sampleRate()));
        }
        m_signal.push_back(w);
        length--;
    }
}

void CGenerator::Sampler()
{
    m_sample.clear();
    const int channelBytes  = m_Format.bytesPerSample();
    const int sampleBytes = m_Format.channelCount() * channelBytes;
    quint64 length = m_signal.length()* sampleBytes;
    m_sample.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_sample.data());
    for (QVector<qreal>::iterator it = m_signal.begin(); it != m_signal.end(); ++it)
    {
        for (int i=0; i<m_Format.channelCount(); ++i)
        {
            switch (m_Format.sampleFormat())
            {
                case QAudioFormat::UInt8:
                    *reinterpret_cast<quint8 *>(ptr) = static_cast<quint8>((1.0 + *it) / 2 * 0xFF);
                    break;
                case QAudioFormat::Int16:
                    *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(*it * 0x7FFF);
                    break;
                case QAudioFormat::Int32:
                    *reinterpret_cast<qint32 *>(ptr) = static_cast<qint32>(*it * std::numeric_limits<qint32>::max());
                    break;
                case QAudioFormat::Float:
                    *reinterpret_cast<float *>(ptr) = *it;
                    break;
                default:
                    break;
            }

            ptr += channelBytes ;
        }
    }
}

void CGenerator::NoiseBlank()
{
    // Pure Noise generator
    for (QVector<qreal>::iterator it = m_signal.begin(); it != m_signal.end(); ++it)
    {
        if ((m_noise_correlation > 0.0) && (m_noise_correlation <=1.0))
        {
            *it = (((m_Rnd.generateDouble()*2.0)-1.0)*m_noise_correlation + (*it * (1.0 - m_noise_correlation)));
        }
    }
}
void CGenerator::LPF()
{
        const int sampleRate = m_Format.sampleRate();
        qreal TimeStep = -100000.0 / static_cast<qreal>(sampleRate);
        //Applying filter
        if (m_Lowfilter_T > 0.0)
        {
            for (QVector<qreal>::iterator it = m_signal.begin(); it != m_signal.end(); ++it)
            {
                qreal Filter = (1- exp(TimeStep/m_Lowfilter_T));
                qreal Diff = *it - m_LastSample;
                qreal Value = m_LastSample + (Diff * Filter);
                *it = Value;
                m_LastSample = *it;
            }
        }
}

void CGenerator::AntiClick()
{
    qreal LastSample = 0.0;
    qreal Threshold = 0.1;
    bool FirstSample =true;
    for (QVector<qreal>::iterator it = m_signal.begin(); it != m_signal.end(); ++it)
    {
        if (!FirstSample)
        {
            qreal diff = *it - LastSample;
            if (qAbs(diff) > Threshold)
            {
                *it = LastSample + (diff * Threshold);
            }
            LastSample = *it;

        }
        else
        {
            FirstSample = false;
            LastSample = *it;
        }
    }
}

void CGenerator::clear()
{
    if (isOpen()) stop();
    m_sample.clear();
    m_sample.resize(0);
}

qint64 CGenerator::readData(char *data, qint64 len)
{
    if (!isOpen()) return -1;
    qint64 total = 0;
    if (!m_sample.isEmpty())
    {
        if (m_loop)
        {
            while (len - total > 0)
            {
                const qint64 chunk = qMin((m_sample.size() - m_pos), len - total);
                memcpy(data + total, m_sample.constData() + m_pos, static_cast<size_t>(chunk));
                m_pos = (m_pos + chunk) % m_sample.size();
                total += chunk;
            }
        }
        else
        {
            const qint64 chunk = qMin((m_sample.size() - m_pos), len);
            if (chunk>0)
            {
                memcpy(data, m_sample.constData() + m_pos, static_cast<size_t>(chunk));
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
        return m_sample.size() + QIODevice::bytesAvailable();
    }
    else
    {
        return m_sample.size()-m_pos;
    }
}

bool CGenerator::atEnd() const
{
    if ((!m_loop) && (m_pos >= m_sample.size())) return true;
    else return false;
}
