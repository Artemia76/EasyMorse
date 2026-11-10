/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
** Contact: https://github.com/Artemia76/EasyMorse
**
** EasyMorse is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** EasyMorse is distributed in the hope that it will be useful,
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
* AudioEngine.cpp is part of EasyMorse project
*
* Audio output based on miniaudio
* **************************************************************************/

#include "AudioEngine.h"

#include <miniaudio.h>

#include "tools/clogger.h"

struct AudioEngine::Device
{
    ma_context  context;
    ma_device   device;
    bool        contextReady = false;
    bool        deviceReady = false;

    static void dataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 pFrameCount)
    {
        static_cast<AudioEngine*>(pDevice->pUserData)->render(static_cast<float*>(pOutput), pFrameCount);
    }
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_device(std::make_unique<Device>())
{
    // EASYMORSE_AUDIO_BACKEND=null gives a silent device running in real time (tests, headless)
    const ma_backend nullBackend[] = {ma_backend_null};
    const bool silent = qEnvironmentVariable("EASYMORSE_AUDIO_BACKEND") == QLatin1String("null");
    if (ma_context_init(silent ? nullBackend : nullptr, silent ? 1 : 0, nullptr, &m_device->context) == MA_SUCCESS)
    {
        m_device->contextReady = true;
        CLogger::instance()->log(QString("AudioEngine: using %1 backend")
                                 .arg(ma_get_backend_name(m_device->context.backend)),
                                 Qt::darkGreen, LEVEL_NORMAL);
    }
    else
    {
        CLogger::instance()->log(tr("Audio: no audio system available"), Qt::red);
    }
    connect(&m_notifyTimer, &QTimer::timeout, this, &AudioEngine::drainNotifications);
}

AudioEngine::~AudioEngine()
{
    stop();
    if (m_device->contextReady)
        ma_context_uninit(&m_device->context);
}

///
/// \brief AudioEngine::outputDevices
/// \return names of the playback devices
///
QStringList AudioEngine::outputDevices()
{
    QStringList devices;
    if (!m_device->contextReady)
        return devices;
    ma_device_info* playbackInfos = nullptr;
    ma_uint32 playbackCount = 0;
    if (ma_context_get_devices(&m_device->context, &playbackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS)
        return devices;
    for (ma_uint32 i = 0; i < playbackCount; ++i)
        devices.append(QString::fromUtf8(playbackInfos[i].name));
    return devices;
}

bool AudioEngine::start(const QString& pDeviceName, int pPeriodMs)
{
    stop();
    if (!m_device->contextReady)
        return false;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SampleRate;
    config.periodSizeInMilliseconds = static_cast<ma_uint32>(qBound(2, pPeriodMs, 200));
    config.performanceProfile = ma_performance_profile_low_latency;
    config.dataCallback = &Device::dataCallback;
    config.pUserData = this;

    // Look for the requested device, fall back to the system default
    ma_device_id deviceId;
    m_deviceName.clear();
    if (!pDeviceName.isEmpty())
    {
        ma_device_info* playbackInfos = nullptr;
        ma_uint32 playbackCount = 0;
        if (ma_context_get_devices(&m_device->context, &playbackInfos, &playbackCount, nullptr, nullptr) == MA_SUCCESS)
        {
            for (ma_uint32 i = 0; i < playbackCount; ++i)
            {
                if (QString::fromUtf8(playbackInfos[i].name) == pDeviceName)
                {
                    deviceId = playbackInfos[i].id;
                    config.playback.pDeviceID = &deviceId;
                    m_deviceName = pDeviceName;
                    break;
                }
            }
        }
        if (m_deviceName.isEmpty())
            CLogger::instance()->log(tr("Audio: device \"%1\" not found, using default output").arg(pDeviceName), Qt::darkMagenta);
    }

    if (ma_device_init(&m_device->context, &config, &m_device->device) != MA_SUCCESS)
    {
        CLogger::instance()->log(tr("Audio: failed to open the output device"), Qt::red);
        return false;
    }
    m_device->deviceReady = true;
    if (ma_device_start(&m_device->device) != MA_SUCCESS)
    {
        CLogger::instance()->log(tr("Audio: failed to start the output device"), Qt::red);
        stop();
        return false;
    }
    if (m_deviceName.isEmpty())
        m_deviceName = QString::fromUtf8(m_device->device.playback.name);
    m_periodMs = static_cast<int>(config.periodSizeInMilliseconds);
    m_notifyTimer.start(10);
    CLogger::instance()->log(QString("AudioEngine: playing on \"%1\", %2 Hz, buffer %3 frames")
                             .arg(m_deviceName)
                             .arg(m_device->device.sampleRate)
                             .arg(m_device->device.playback.internalPeriodSizeInFrames),
                             Qt::darkGreen, LEVEL_NORMAL);
    return true;
}

void AudioEngine::stop()
{
    if (m_device->deviceReady)
    {
        ma_device_uninit(&m_device->device);
        m_device->deviceReady = false;
    }
    m_notifyTimer.stop();
    drainNotifications();
}

bool AudioEngine::isRunning() const
{
    return m_device->deviceReady;
}

QString AudioEngine::deviceName() const
{
    return m_deviceName;
}

/*
IMMEDIATE COMMANDS
*/

void AudioEngine::post(const AudioEvent& pEvent)
{
    if (!m_immediate.push(pEvent))
        CLogger::instance()->log("AudioEngine: command queue full", Qt::red, LEVEL_VERBOSE);
}

void AudioEngine::keyDown(KeySource pSource)
{
    post({AudioEvent::Type::KeyDown, 0, 0.0, static_cast<int>(pSource), 0});
}

void AudioEngine::keyUp(KeySource pSource)
{
    post({AudioEvent::Type::KeyUp, 0, 0.0, static_cast<int>(pSource), 0});
}

void AudioEngine::setFrequency(double pFrequency)
{
    post({AudioEvent::Type::Frequency, 0, pFrequency, 0, 0});
}

void AudioEngine::setGain(int pPercent)
{
    post({AudioEvent::Type::Gain, 0, static_cast<double>(pPercent), 0, 0});
}

void AudioEngine::setBand(band::BandSimulator::Parameter pParameter, double pValue)
{
    post({AudioEvent::Type::Band, 0, pValue, static_cast<int>(pParameter), 0});
}

bool AudioEngine::sendStation(const band::StationScript& pScript)
{
    return m_stations.push(pScript);
}

/*
SCHEDULING
*/

qint64 AudioEngine::framesRendered() const
{
    return m_framesRendered.load(std::memory_order_acquire);
}

///
/// \brief AudioEngine::latencyFrames
/// \return safe delay between now and the first frame that can still be scheduled
///
qint64 AudioEngine::latencyFrames() const
{
    return 3LL * m_periodMs * SampleRate / 1000;
}

quint32 AudioEngine::generation() const
{
    return m_generation;
}

bool AudioEngine::schedule(const AudioEvent& pEvent)
{
    return m_scheduled.push(pEvent);
}

quint32 AudioEngine::cancelScheduled()
{
    ++m_generation;
    post({AudioEvent::Type::Cancel, 0, 0.0, 0, m_generation});
    return m_generation;
}

void AudioEngine::drainNotifications()
{
    while (auto event = m_notifications.pop())
    {
        if (event->type == AudioEvent::Type::StationFinished)
            emit stationFinished(event->id);
        else
            emit markerReached(event->generation, event->id);
    }
}

/*
AUDIO THREAD
*/

void AudioEngine::apply(const AudioEvent& pEvent)
{
    switch (pEvent.type)
    {
    case AudioEvent::Type::KeyDown:
    case AudioEvent::Type::KeyUp:
        if (pEvent.id >= 0 && pEvent.id < static_cast<int>(KeySource::Count))
            m_keyDown[pEvent.id] = (pEvent.type == AudioEvent::Type::KeyDown);
        break;
    case AudioEvent::Type::Frequency:
        m_band.setFrequency(pEvent.value);
        break;
    case AudioEvent::Type::Gain:
        // 100 % gives the level of the previous versions
        m_band.setGain(pEvent.value / 1000.0);
        break;
    case AudioEvent::Type::Band:
        m_band.set(static_cast<band::BandSimulator::Parameter>(pEvent.id), pEvent.value);
        break;
    case AudioEvent::Type::StationFinished:
        break;
    case AudioEvent::Type::Cancel:
        m_cancelGeneration = pEvent.generation;
        m_keyDown[static_cast<int>(KeySource::Sequencer)] = false;
        break;
    case AudioEvent::Type::Marker:
        m_notifications.push(pEvent);
        break;
    }

    m_band.setSidetone(m_keyDown[static_cast<int>(KeySource::Local)]);
    m_band.setSignal(m_keyDown[static_cast<int>(KeySource::Sequencer)]
                        || m_keyDown[static_cast<int>(KeySource::Remote)]);
}

void AudioEngine::render(float* pOutput, unsigned int pFrameCount)
{
    // Commands from the GUI apply at the start of the buffer
    while (auto event = m_immediate.pop())
        apply(*event);
    while (const band::StationScript* script = m_stations.front())
    {
        m_band.play(*script);
        m_stations.pop();
    }

    for (unsigned int i = 0; i < pFrameCount; ++i, ++m_frame)
    {
        // Scheduled events apply at their exact frame (or now if late)
        while (const AudioEvent* event = m_scheduled.front())
        {
            if (event->generation < m_cancelGeneration)
            {
                m_scheduled.pop();
                continue;
            }
            if (event->frame > m_frame)
                break;
            apply(*event);
            m_scheduled.pop();
        }
        const float value = m_band.next();
        *pOutput++ = value; // left
        *pOutput++ = value; // right
    }
    for (int slot = m_band.takeFinished(); slot >= 0; slot = m_band.takeFinished())
        m_notifications.push({AudioEvent::Type::StationFinished, 0, 0.0, slot, 0});
    m_framesRendered.store(m_frame, std::memory_order_release);
}
