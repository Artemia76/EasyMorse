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
* AudioEngine.h is part of EasyMorse project
*
* Audio output based on miniaudio. The radio channel (tone, noise, fading)
* only lives in the real time audio thread: the GUI thread talks to it through lock-free queues,
* either with immediate commands (keyer, settings) or with events scheduled
* at an exact frame (Morse playback).
* **************************************************************************/

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QStringList>
#include <QTimer>

#include <atomic>
#include <memory>

#include "audio/radiochannel.h"
#include "tools/spscqueue.h"

///
/// \brief Who is pressing the key. The local key sounds as a clean sidetone,
/// the others as the received signal (with QSB).
///
enum class KeySource : int
{
    Local = 0,      // keyboard, serial key
    Sequencer = 1,  // text played by the application
    Remote = 2,     // station connected through the network
    Count
};

struct AudioEvent
{
    enum class Type
    {
        KeyDown,        // source in id
        KeyUp,          // source in id
        Frequency,      // Hz in value
        Gain,           // 0..100 in value
        NoiseRatio,     // 0..1 in value
        Bandwidth,      // Hz in value
        QsbDepth,       // 0..100 in value
        Cancel,         // drop scheduled events older than generation
        Marker          // notify the GUI with id when reached
    };
    Type        type = Type::Marker;
    qint64      frame = 0;          // scheduled events only
    double      value = 0.0;
    int         id = 0;
    quint32     generation = 0;     // scheduled events only
};

class AudioEngine : public QObject
{
    Q_OBJECT

public:
    static constexpr quint32 SampleRate = 48000;

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    QStringList outputDevices();
    ///
    /// \brief start the output stream
    /// \param pDeviceName device name from outputDevices(), empty for system default
    /// \param pPeriodMs size of an audio buffer, i.e. keyer latency
    ///
    bool        start(const QString& pDeviceName = QString(), int pPeriodMs = 10);
    void        stop();
    bool        isRunning() const;
    QString     deviceName() const;

    // Immediate commands, GUI thread only
    void        keyDown(KeySource pSource);
    void        keyUp(KeySource pSource);
    void        setFrequency(double pFrequency);
    void        setGain(int pPercent);
    void        setNoiseRatio(double pRatio);
    void        setBandwidth(int pHz);
    void        setQsbDepth(int pPercent);

    // Scheduling, GUI thread only
    qint64      framesRendered() const;
    qint64      latencyFrames() const;
    quint32     generation() const;
    bool        schedule(const AudioEvent& pEvent);
    ///
    /// \brief cancelScheduled drops every scheduled event and releases the sequencer key
    /// \return the new generation to use for events scheduled afterwards
    ///
    quint32     cancelScheduled();

signals:
    void        markerReached(quint32 pGeneration, int pId);

private:
    struct Device;

    void        render(float* pOutput, unsigned int pFrameCount);
    void        apply(const AudioEvent& pEvent);
    void        post(const AudioEvent& pEvent);
    void        drainNotifications();

    std::unique_ptr<Device>         m_device;
    QString                         m_deviceName;
    int                             m_periodMs = 10;
    quint32                         m_generation = 0;
    QTimer                          m_notifyTimer;

    SpscQueue<AudioEvent, 1024>     m_immediate;
    SpscQueue<AudioEvent, 4096>     m_scheduled;
    SpscQueue<AudioEvent, 1024>     m_notifications;
    std::atomic<qint64>             m_framesRendered{0};

    // Audio thread state
    qint64                          m_frame = 0;
    quint32                         m_cancelGeneration = 0;
    bool                            m_keyDown[static_cast<int>(KeySource::Count)] = {};
    RadioChannel                    m_channel{SampleRate};
};

#endif // AUDIOENGINE_H
