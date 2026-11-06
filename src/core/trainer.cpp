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
* trainer.cpp is part of EasyMorse project
* **************************************************************************/

#include "trainer.h"

#include "settings/settings.h"
#include "tools/clogger.h"
#ifdef EASYMORSE_HAS_SERIALPORT
#include "keyer/serialkeyer.h"
#endif

Trainer::Trainer(Settings* pSettings, QObject* parent)
    : QObject(parent)
    , m_settings(pSettings)
    , m_sequencer(&m_audio)
{
    // Playback
    connect(&m_sequencer, &MorseSequencer::charPlayed, this, &Trainer::charPlayed);
    connect(&m_sequencer, &MorseSequencer::finished, this, [this] {
        if (!m_playing)
            return;
        m_playing = false;
        emit playingChanged();
    });

    // Decoder
    connect(&m_decoder, &KeyDecoder::charDecoded, this, [this](const QString& pText, const QString& pPattern) {
        m_decoderPattern.clear();
        emit decoderPatternChanged();
        emit charDecoded(pText, pPattern);
    });
    connect(&m_decoder, &KeyDecoder::wordGap, this, &Trainer::wordGap);
    connect(&m_decoder, &KeyDecoder::symbolProgress, this, [this](const QString& pPattern) {
        m_decoderPattern = pPattern;
        emit decoderPatternChanged();
    });
    connect(&m_decoder, &KeyDecoder::speedChanged, this, &Trainer::decoderSpeedChanged);

    // Keys
    addKeyer(&m_manualKeyer);
#ifdef EASYMORSE_HAS_SERIALPORT
    m_serialKeyer = new SerialKeyer(this);
    addKeyer(m_serialKeyer);
    connect(m_serialKeyer, &SerialKeyer::errorOccurred, this, [](const QString& pMessage) {
        CLogger::instance()->log(pMessage, Qt::red);
    });
#endif

    // Remote station: played as its own key source and decoded by its own
    // decoder, so its speed is learnt separately
    connect(m_session.remoteKeyer(), &KeyerInput::keyChanged, this, &Trainer::onRemoteKeyChanged);
    connect(&m_remoteDecoder, &KeyDecoder::charDecoded, this, &Trainer::remoteCharDecoded);
    connect(&m_remoteDecoder, &KeyDecoder::wordGap, this, &Trainer::remoteWordGap);
    connect(&m_remoteDecoder, &KeyDecoder::speedChanged, this, &Trainer::remoteDecoderSpeedChanged);
    m_remoteDecoder.setSpeed(m_settings->charSpeed());
    // The message is considered over after this long without keying
    m_remoteIdleTimer.setSingleShot(true);
    m_remoteIdleTimer.setInterval(3000);
    connect(&m_remoteIdleTimer, &QTimer::timeout, this, [this] {
        m_remoteActive = false;
        emit remoteActiveChanged();
    });
    connect(&m_session, &PeerSession::stateChanged, this, [this] {
        if (m_session.state() == PeerSession::State::Connected)
            return;
        m_remoteDecoder.clear();
        m_remoteIdleTimer.stop();
        if (m_remoteActive)
        {
            m_remoteActive = false;
            emit remoteActiveChanged();
        }
    });
    applySessionSettings();
    connect(m_settings, &Settings::callsignChanged, this, &Trainer::applySessionSettings);
    connect(m_settings, &Settings::stunServerChanged, this, &Trainer::applySessionSettings);
    connect(m_settings, &Settings::jitterDelayMsChanged, this, &Trainer::applySessionSettings);

    // Sound follows the settings
    m_audio.setFrequency(m_settings->toneFrequency());
    m_audio.setGain(m_settings->volume());
    m_audio.setNoiseRatio(m_settings->noiseLevel());
    m_audio.setNoiseCutoff(m_settings->noiseFilter());
    m_audio.noiseOn();
    connect(m_settings, &Settings::toneFrequencyChanged, this, [this] { m_audio.setFrequency(m_settings->toneFrequency()); });
    connect(m_settings, &Settings::volumeChanged, this, [this] { m_audio.setGain(m_settings->volume()); });
    connect(m_settings, &Settings::noiseLevelChanged, this, [this] { m_audio.setNoiseRatio(m_settings->noiseLevel()); });
    connect(m_settings, &Settings::noiseFilterChanged, this, [this] { m_audio.setNoiseCutoff(m_settings->noiseFilter()); });

    // Device changes are grouped: the options dialog changes several at once
    const auto scheduleAudioRestart = [this] {
        if (m_audioRestartPending)
            return;
        m_audioRestartPending = true;
        QMetaObject::invokeMethod(this, &Trainer::restartAudio, Qt::QueuedConnection);
    };
    connect(m_settings, &Settings::audioDeviceChanged, this, scheduleAudioRestart);
    connect(m_settings, &Settings::audioPeriodMsChanged, this, scheduleAudioRestart);
    const auto scheduleSerialReopen = [this] {
        if (m_serialReopenPending)
            return;
        m_serialReopenPending = true;
        QMetaObject::invokeMethod(this, &Trainer::reopenSerial, Qt::QueuedConnection);
    };
    connect(m_settings, &Settings::serialEnabledChanged, this, scheduleSerialReopen);
    connect(m_settings, &Settings::serialPortChanged, this, scheduleSerialReopen);

    // Decoder starts from the speed learnt in the previous session
    m_decoder.setSpeed(m_settings->charSpeed());
    if (m_settings->decoderAutoAdapt())
    {
        double dot, dash, gap;
        m_settings->decoderEstimates(dot, dash, gap);
        m_decoder.setEstimates(dot, dash, gap);
    }
    applyDecoderMode();
    connect(m_settings, &Settings::decoderAutoAdaptChanged, this, &Trainer::applyDecoderMode);
    connect(m_settings, &Settings::decoderReactivityChanged, this, &Trainer::applyDecoderMode);
    connect(m_settings, &Settings::charSpeedChanged, this, &Trainer::applyDecoderMode);
}

Trainer::~Trainer()
{
    m_settings->setDecoderEstimates(m_decoder.dotMs(), m_decoder.dashMs(), m_decoder.gapMs());
}

void Trainer::start()
{
    restartAudio();
    reopenSerial();
}

void Trainer::addKeyer(KeyerInput* pKeyer)
{
    if (m_keyers.contains(pKeyer))
        return;
    m_keyers.insert(pKeyer);
    connect(pKeyer, &KeyerInput::keyChanged, this, &Trainer::onKeyChanged);
}

void Trainer::removeKeyer(KeyerInput* pKeyer)
{
    if (!m_keyers.remove(pKeyer))
        return;
    disconnect(pKeyer, &KeyerInput::keyChanged, this, &Trainer::onKeyChanged);
    if (m_pressed.remove(pKeyer) && m_pressed.isEmpty())
        setKeyDown(false, MonotonicClock::nowMs());
}

QStringList Trainer::audioDevices()
{
    return m_audio.outputDevices();
}

QStringList Trainer::serialPorts() const
{
#ifdef EASYMORSE_HAS_SERIALPORT
    return SerialKeyer::availablePorts();
#else
    return {};
#endif
}

bool Trainer::play(const QString& pText)
{
    const MorseTiming timing = MorseTiming::fromSpeed(m_settings->wordSpeed(),
                                                      m_settings->charSpeed(),
                                                      m_settings->farnsworth());
    if (!m_sequencer.play(pText, timing))
    {
        CLogger::instance()->log(tr("Nothing to play, or no sound output"), Qt::darkMagenta);
        return false;
    }
    // The keys are muted while a text is played
    if (m_keyDown)
        setKeyDown(false, MonotonicClock::nowMs());
    m_playing = true;
    emit playingChanged();
    return true;
}

void Trainer::stop()
{
    m_sequencer.stop();
}

bool Trainer::isPlaying() const
{
    return m_playing;
}

bool Trainer::isAudioRunning() const
{
    return m_audio.isRunning();
}

double Trainer::decoderSpeed() const
{
    return m_decoder.speed();
}

///
/// \brief Trainer::onKeyChanged combines the keys: the tone sounds while any of them is down
///
void Trainer::onKeyChanged(bool pDown, qint64 pTimeMs)
{
    auto* keyer = qobject_cast<KeyerInput*>(sender());
    if (pDown)
        m_pressed.insert(keyer);
    else
        m_pressed.remove(keyer);
    if (m_playing)
        return;
    const bool down = !m_pressed.isEmpty();
    if (down != m_keyDown)
        setKeyDown(down, pTimeMs);
}

void Trainer::setKeyDown(bool pDown, qint64 pTimeMs)
{
    m_keyDown = pDown;
    if (pDown)
        m_audio.keyDown(KeySource::Local);
    else
        m_audio.keyUp(KeySource::Local);
    m_decoder.keyEdge(pDown, pTimeMs);
    // Sent only when a session is connected
    m_session.sendKey(pDown, pTimeMs);
    emit keyDownChanged();
}

bool Trainer::isRemoteKeyDown() const
{
    return m_session.state() == PeerSession::State::Connected && m_session.remoteKeyer()->isDown();
}

double Trainer::remoteDecoderSpeed() const
{
    return m_remoteDecoder.speed();
}

void Trainer::onRemoteKeyChanged(bool pDown, qint64 pTimeMs)
{
    if (pDown)
        m_audio.keyDown(KeySource::Remote);
    else
        m_audio.keyUp(KeySource::Remote);
    m_remoteDecoder.keyEdge(pDown, pTimeMs);
    if (pDown)
    {
        m_remoteIdleTimer.stop();
        if (!m_remoteActive)
        {
            m_remoteActive = true;
            emit remoteActiveChanged();
        }
    }
    else
    {
        m_remoteIdleTimer.start();
    }
    emit remoteKeyDownChanged();
}

void Trainer::applySessionSettings()
{
    m_session.setCallsign(m_settings->callsign());
    m_session.setJitterDelay(m_settings->jitterDelayMs());
    // "host:port", 3478 (standard STUN port) when the port is missing
    PeerLink::Server stun;
    const QString server = m_settings->stunServer();
    const qsizetype colon = server.lastIndexOf(u':');
    bool portOk = false;
    const int port = colon > 0 ? server.mid(colon + 1).toInt(&portOk) : 0;
    stun.host = (colon > 0 && portOk) ? server.left(colon) : server;
    stun.port = static_cast<quint16>((portOk && port > 0 && port < 65536) ? port : 3478);
    m_session.setStunServer(stun);
}

void Trainer::restartAudio()
{
    m_audioRestartPending = false;
    stop();
    if (!m_audio.start(m_settings->audioDevice(), m_settings->audioPeriodMs()))
        CLogger::instance()->log(tr("No sound output, check the audio options"), Qt::red);
    emit audioRunningChanged();
}

void Trainer::reopenSerial()
{
    m_serialReopenPending = false;
#ifdef EASYMORSE_HAS_SERIALPORT
    m_serialKeyer->close();
    if (!m_settings->serialEnabled())
        return;
    if (m_settings->serialPort().isEmpty())
    {
        CLogger::instance()->log(tr("Serial key enabled but no port chosen"), Qt::darkMagenta);
        return;
    }
    m_serialKeyer->open(m_settings->serialPort());
#endif
}

void Trainer::applyDecoderMode()
{
    const bool autoAdapt = m_settings->decoderAutoAdapt();
    m_decoder.setAutoAdapt(autoAdapt);
    m_decoder.setReactivity(static_cast<KeyDecoder::Reactivity>(m_settings->decoderReactivity()));
    // The remote speed is unknown: its decoder always adapts
    m_remoteDecoder.setReactivity(static_cast<KeyDecoder::Reactivity>(m_settings->decoderReactivity()));
    // A locked decoder expects the character speed
    if (!autoAdapt)
        m_decoder.setSpeed(m_settings->charSpeed());
}
