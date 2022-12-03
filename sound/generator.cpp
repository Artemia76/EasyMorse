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
#include "tools/singleton.h"

#include <QMediaDevices>
#include <QAudioOutput>
#include <QDebug>
#include <QVBoxLayout>
#include <qmath.h>
#include <qendian.h>
#include <QSysInfo>
#include <math.h>

CGenerator::CGenerator(QObject* parent) :
    QIODevice(parent)
{

    m_Freq = 900;
    m_FadeTime = 1000;
    m_Amplitude =0.0;
    m_log = CLogger::instance();
    m_noise_correlation=0.0;
    m_LastSample=0.0;
    m_Lowfilter_T=0;
    m_phi = 0;
}

///
/// \brief CGenerator::createInstance
/// \return
///
CGenerator* CGenerator::createInstance()
{
    return new CGenerator();
}

///
/// \brief CGenerator::instance
/// \return#include "singleton.h"
///
CGenerator* CGenerator::instance()
{
    return Singleton<CGenerator>::instance(CGenerator::createInstance);
}

void CGenerator::setDevice(const QAudioDevice &deviceInfo)
{
    stop();
    m_audioOutput.reset(new QAudioSink(deviceInfo, deviceInfo.preferredFormat()));
    m_Format = deviceInfo.preferredFormat();
}

void CGenerator::start()
{
    if (!m_audioOutput.isNull())
    {
        connect(m_audioOutput.data(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(onOutputAudioStateChanged(QAudio::State)));
        if (!open(QIODevice::ReadOnly))
        {
            m_log->log(tr("Error openning sound output"),Qt::magenta);
            return;
        }
        //m_audioOutput->setBufferSize(256);
        m_audioOutput->start(this);
    }
}

void CGenerator::stop()
{
    if (!m_audioOutput.isNull())
    {
        m_audioOutput->stop();
        close();
        disconnect(m_audioOutput.data(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(onOutputAudioStateChanged(QAudio::State)));
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

void CGenerator::setVolume(int pVolume)
{
    m_Volume = QAudio::convertVolume(pVolume / qreal(100),
        QAudio::LogarithmicVolumeScale,
        QAudio::LinearVolumeScale);
        if (!m_audioOutput.isNull())
            m_audioOutput->setVolume(m_Volume);
}

int CGenerator::getVolume()
{
    return m_Freq;
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

qint64 CGenerator::_sampler(qint64 pLen)
{
    const int sampleRate = m_Format.sampleRate();
    qreal TimeStep = -100000.0 / static_cast<qreal>(sampleRate);
    quint64 length = pLen * _getSampleBytes();
    qreal w = 0.0; //Wave
    m_sample.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_sample.data());
    int old_phi = m_phi;
    for (int i=old_phi; i< pLen+old_phi; i++)
    {
        m_phi = i % m_Format.sampleRate();
        // Wave generator in function of Samplerate and frequency
        w = qSin(2.0 * M_PI * static_cast<qreal>(m_Freq) * static_cast<qreal>(m_phi++) / static_cast<qreal>(m_Format.sampleRate()));
        // Pure Noise generator
        if ((m_noise_correlation > 0.0) && (m_noise_correlation <=1.0))
        {
            w = (((m_Rnd.generateDouble()*2.0)-1.0)*m_noise_correlation + (w * (1.0 - m_noise_correlation)));
        }
        // Apply volume
        w *= m_Volume;
        if (m_Lowfilter_T > 0.0)
        {
            qreal Filter = (1- exp(TimeStep/m_Lowfilter_T));
            qreal Diff = w - m_LastSample;
            qreal Value = m_LastSample + (Diff * Filter);
            w = Value;
            m_LastSample = w;
        }
        for (int i=0; i<m_Format.channelCount(); ++i)
        {
            switch (m_Format.sampleFormat())
            {
                case QAudioFormat::UInt8:
                    *reinterpret_cast<quint8 *>(ptr) = static_cast<quint8>((1.0 + w) / 2 * 0xFF);
                    break;
                case QAudioFormat::Int16:
                    *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(w * 0x7FFF);
                    break;
                case QAudioFormat::Int32:
                    *reinterpret_cast<qint32 *>(ptr) = static_cast<qint32>(w * std::numeric_limits<qint32>::max());
                    break;
                case QAudioFormat::Float:
                    *reinterpret_cast<float *>(ptr) = w;
                    break;
                default:
                    break;
            }

            ptr += m_Format.bytesPerSample() ;
        }
    }
    return m_sample.length();
}

int CGenerator::_getSampleBytes()
{
    return m_Format.channelCount() * m_Format.bytesPerSample();
}

///
/// \brief CGenerator::readData
/// \param data
/// \param len
/// \return
///
/// Generate jit signal
///
qsizetype CGenerator::readData(char *data, qint64 len)
{
    if (!isOpen()) return -1;
    if (len < _getSampleBytes()) return -1;
    qint64 SignalLen = len / _getSampleBytes();
    //qint64 SampleLen = SignalLen *  _getSampleBytes();
    qint64 SampleLen = _sampler(SignalLen);
    memcpy(data, m_sample.constData(), static_cast<size_t>(SampleLen));
    return SampleLen;
}

qint64 CGenerator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 CGenerator::pos() const
{
    return 0;
}

qint64 CGenerator::bytesAvailable() const
{
    return 100000;
}

bool CGenerator::atEnd() const
{
    return false;
}

///
/// \brief CGenerator::onOutputAudioStateChanged
/// \param pNewState
///
void CGenerator::onOutputAudioStateChanged(QAudio::State pNewState)
{
    switch (pNewState)
    {
        case QAudio::StoppedState:
        {
#ifdef QT_DEBUG
            m_log->log("Audio now in stopped state",Qt::magenta,LEVEL_VERBOSE);
#endif
        }
        case QAudio::SuspendedState:
        {
#ifdef QT_DEBUG
            m_log->log("Audio now in suspended state",Qt::magenta,LEVEL_VERBOSE);
#endif
            break;
        }
        case QAudio::IdleState:
        {
#ifdef QT_DEBUG
            m_log->log("Audio now in Idle state",Qt::magenta,LEVEL_VERBOSE);
#endif
            //if (m_playing_phrase) StopMorseMessage();
            break;
        }
        case QAudio::ActiveState:
        {
    #ifdef QT_DEBUG
            m_log->log("Audio now in Active state",Qt::magenta,LEVEL_VERBOSE);
    #endif
            break;
        }
        default:
        {
#ifdef QT_DEBUG
            m_log->log("Audio now in unknown state",Qt::magenta,LEVEL_VERBOSE);
#endif
            break;
        }
    }
}
