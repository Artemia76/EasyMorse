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
    connect(&m_sequencer, &MorseSequencer::charPlayed, this, [this](int pIndex) {
        emit charPlayed(pIndex);
        if (pIndex > m_playShown && pIndex < m_playText.size())
        {
            emit charSent(m_playText.mid(m_playShown + 1, pIndex - m_playShown).toUpper());
            m_playShown = pIndex;
        }
    });
    m_playKeyTimer.setSingleShot(true);
    m_playKeyTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_playKeyTimer, &QTimer::timeout, this, &Trainer::sendPlayedKeys);
    connect(&m_sequencer, &MorseSequencer::finished, this, [this] {
        endPlayedKeys();
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

    // Remote stations: the manual session and the members of the room, all
    // played as the remote key source
    // The message is considered over after this long without keying
    m_remoteIdleTimer.setSingleShot(true);
    m_remoteIdleTimer.setInterval(3000);
    connect(&m_remoteIdleTimer, &QTimer::timeout, this, [this] {
        m_remoteActive = false;
        emit remoteActiveChanged();
    });
    addRemote(&m_session);
    connect(&m_lobby, &Lobby::peerAdded, this, &Trainer::addRemote);
    connect(&m_lobby, &Lobby::peerRemoved, this, &Trainer::removeRemote);
    applySessionSettings();
    SignalingClient* client = m_lobby.client();
    client->setServerUrl(QUrl(m_settings->serverUrl()));
    connect(m_settings, &Settings::serverUrlChanged, this, [this, client] {
        client->setServerUrl(QUrl(m_settings->serverUrl()));
    });
    connect(client, &SignalingClient::tokenChanged, m_settings, &Settings::setServerToken);
    // Logged in: the account callsign is the station callsign
    connect(client, &SignalingClient::stateChanged, this, [this, client] {
        if (client->state() == SignalingClient::State::Online)
            m_settings->setCallsign(client->callsign());
    });
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
    // Members are destroyed in reverse order: the session, closing, would
    // report to slots using the remote decoder and timer already destroyed
    disconnect(&m_lobby, nullptr, this, nullptr);
    for (const auto& [session, decoder] : m_remoteDecoders)
    {
        disconnect(session, nullptr, this, nullptr);
        disconnect(session->remoteKeyer(), nullptr, this, nullptr);
    }
    m_session.stop();
    m_settings->setDecoderEstimates(m_decoder.dotMs(), m_decoder.dashMs(), m_decoder.gapMs());
}

void Trainer::start()
{
    restartAudio();
    reopenSerial();
    // Back on the match-making server with the token of the last login
    if (!m_settings->serverUrl().isEmpty() && !m_settings->serverToken().isEmpty())
        m_lobby.client()->resume(m_settings->serverToken());
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
    // The correspondents hear the text too
    endPlayedKeys();
    m_playText = pText;
    m_playShown = -1;
    for (const TimelineEvent& event : MorseSequencer::buildTimeline(pText, timing))
    {
        if (event.kind == TimelineEvent::Kind::KeyDown || event.kind == TimelineEvent::Kind::KeyUp)
            m_playEdges.append(event);
    }
    m_playStartMs = MonotonicClock::nowMs();
    sendPlayedKeys();
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
    sendKey(pDown, pTimeMs);
    emit keyDownChanged();
}

void Trainer::sendKey(bool pDown, qint64 pTimeMs)
{
    // Sent only to the connected sessions
    m_session.sendKey(pDown, pTimeMs);
    m_lobby.sendKey(pDown, pTimeMs);
}

void Trainer::sendPlayedKeys()
{
    const qint64 now = MonotonicClock::nowMs();
    while (m_playNext < m_playEdges.size())
    {
        const TimelineEvent& edge = m_playEdges.at(m_playNext);
        // The time of the timeline, not the time the timer fired: exact rhythm
        const qint64 time = m_playStartMs + qRound64(edge.time * 1000.0);
        if (time > now)
        {
            m_playKeyTimer.start(static_cast<int>(time - now));
            return;
        }
        m_playKeyDown = edge.kind == TimelineEvent::Kind::KeyDown;
        sendKey(m_playKeyDown, time);
        ++m_playNext;
    }
}

void Trainer::endPlayedKeys()
{
    m_playKeyTimer.stop();
    m_playEdges.clear();
    m_playNext = 0;
    // Stopped in the middle of a sign
    if (m_playKeyDown)
    {
        m_playKeyDown = false;
        sendKey(false, MonotonicClock::nowMs());
    }
}

double Trainer::stationSpeed(PeerSession* pSession) const
{
    const auto it = m_remoteDecoders.find(pSession);
    return it == m_remoteDecoders.end() ? 0.0 : it->second->speed();
}

bool Trainer::isRemoteKeyDown() const
{
    return !m_remoteDown.isEmpty();
}

double Trainer::remoteDecoderSpeed() const
{
    const auto it = m_remoteDecoders.find(m_lastRemote);
    return it == m_remoteDecoders.end() ? 0.0 : it->second->speed();
}

QString Trainer::remoteName(PeerSession* pSession) const
{
    return pSession->peerCallsign().isEmpty() ? tr("Correspondent") : pSession->peerCallsign();
}

void Trainer::addRemote(PeerSession* pSession)
{
    auto decoder = std::make_unique<KeyDecoder>();
    decoder->setSpeed(m_settings->charSpeed());
    decoder->setReactivity(static_cast<KeyDecoder::Reactivity>(m_settings->decoderReactivity()));
    connect(decoder.get(), &KeyDecoder::charDecoded, this, [this, pSession](const QString& pText, const QString& pPattern) {
        emit remoteCharDecoded(pSession, pText, pPattern);
    });
    connect(decoder.get(), &KeyDecoder::wordGap, this, [this, pSession] { emit remoteWordGap(pSession); });
    connect(decoder.get(), &KeyDecoder::speedChanged, this, [this, pSession] {
        emit remoteSpeedChanged(pSession);
        if (pSession == m_lastRemote)
            emit remoteDecoderSpeedChanged();
    });
    m_remoteDecoders[pSession] = std::move(decoder);
    connect(pSession->remoteKeyer(), &KeyerInput::keyChanged, this, [this, pSession](bool pDown, qint64 pTimeMs) {
        onRemoteKeyChanged(pSession, pDown, pTimeMs);
    });
    connect(pSession, &PeerSession::stateChanged, this, [this, pSession] {
        if (pSession->state() != PeerSession::State::Connected)
            silenceRemote(pSession);
    });
}

void Trainer::removeRemote(PeerSession* pSession)
{
    disconnect(pSession, nullptr, this, nullptr);
    disconnect(pSession->remoteKeyer(), nullptr, this, nullptr);
    silenceRemote(pSession);
    m_remoteDecoders.erase(pSession);
    if (m_lastRemote == pSession)
        m_lastRemote = nullptr;
}

void Trainer::silenceRemote(PeerSession* pSession)
{
    if (m_remoteDown.remove(pSession))
    {
        if (m_remoteDown.isEmpty())
            m_audio.keyUp(KeySource::Remote);
        emit remoteKeyDownChanged();
    }
    const auto it = m_remoteDecoders.find(pSession);
    if (it != m_remoteDecoders.end())
        it->second->clear();
    if (m_remoteDown.isEmpty() && m_remoteActive)
    {
        m_remoteIdleTimer.start();
    }
}

///
/// \brief Trainer::onRemoteKeyChanged the tone sounds while any remote key is down,
/// as on a shared frequency
///
void Trainer::onRemoteKeyChanged(PeerSession* pSession, bool pDown, qint64 pTimeMs)
{
    const bool wasDown = !m_remoteDown.isEmpty();
    if (pDown)
        m_remoteDown.insert(pSession);
    else
        m_remoteDown.remove(pSession);
    const bool down = !m_remoteDown.isEmpty();
    if (down && !wasDown)
        m_audio.keyDown(KeySource::Remote);
    else if (!down && wasDown)
        m_audio.keyUp(KeySource::Remote);

    const auto it = m_remoteDecoders.find(pSession);
    if (it != m_remoteDecoders.end())
        it->second->keyEdge(pDown, pTimeMs);
    if (pDown)
    {
        if (pSession != m_lastRemote || m_remoteCallsign != remoteName(pSession))
        {
            m_lastRemote = pSession;
            m_remoteCallsign = remoteName(pSession);
            emit remoteDecoderSpeedChanged();
        }
        m_remoteIdleTimer.stop();
        if (!m_remoteActive)
        {
            m_remoteActive = true;
            emit remoteActiveChanged();
        }
    }
    else if (!down)
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
    m_session.setTurnServer({});
    m_lobby.setFallbackStun(stun);
    m_lobby.setJitterDelay(m_settings->jitterDelayMs());
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
    // The remote speeds are unknown: their decoders always adapt
    for (const auto& [session, decoder] : m_remoteDecoders)
        decoder->setReactivity(static_cast<KeyDecoder::Reactivity>(m_settings->decoderReactivity()));
    // A locked decoder expects the character speed
    if (!autoAdapt)
        m_decoder.setSpeed(m_settings->charSpeed());
}
