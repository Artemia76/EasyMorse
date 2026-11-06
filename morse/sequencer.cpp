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
* sequencer.cpp is part of EasyMorse project
*
* Turns a text into Morse key events and plays it through the audio engine
* **************************************************************************/

#include "sequencer.h"

#include <cmath>

#include "audio/AudioEngine.h"
#include "morse/morsecodec.h"

namespace
{
    // Events are handed to the audio thread this far in advance
    constexpr double FeedHorizon = 0.5;
}

MorseTiming MorseTiming::fromSpeed(int pWordSpeed, int pCharSpeed, bool pFarnsworth)
{
    const double wordSpeed = qMax(1, pWordSpeed);
    const double charSpeed = qMax(1, pCharSpeed);
    MorseTiming timing;
    if (pFarnsworth && (wordSpeed < charSpeed))
    {
        // ARRL Farnsworth timing: characters sent at charSpeed, extra delay
        // spread over the character and word spaces to get wordSpeed overall
        timing.unit = 1.2 / charSpeed;
        const double delay = (60.0 * charSpeed - 37.2 * wordSpeed) / (wordSpeed * charSpeed);
        timing.charGap = 3.0 * delay / 19.0;
        timing.wordGap = 7.0 * delay / 19.0;
    }
    else
    {
        timing.unit = 1.2 / wordSpeed;
        timing.charGap = 3.0 * timing.unit;
        timing.wordGap = 7.0 * timing.unit;
    }
    return timing;
}

///
/// \brief MorseSequencer::buildTimeline
/// \details Characters without Morse code are skipped. The End event comes
/// after the trailing space, so "PARIS " lasts exactly 50 units.
///
QList<TimelineEvent> MorseSequencer::buildTimeline(const QString& pText, const MorseTiming& pTiming)
{
    QList<TimelineEvent> timeline;
    double time = 0.0;
    double pendingGap = 0.0;    // silence owed before the next character
    bool started = false;
    for (qsizetype i = 0; i < pText.size(); ++i)
    {
        const QChar character = pText.at(i);
        if (character.isSpace())
        {
            if (started)
                pendingGap = qMax(pendingGap, pTiming.wordGap);
            continue;
        }
        const QString pattern = MorseCodec::encode(character);
        if (pattern.isEmpty())
            continue;
        time += pendingGap;
        timeline.append({TimelineEvent::Kind::CharStart, time, static_cast<int>(i)});
        for (qsizetype e = 0; e < pattern.size(); ++e)
        {
            if (e > 0)
                time += pTiming.unit;
            timeline.append({TimelineEvent::Kind::KeyDown, time});
            time += (pattern.at(e) == u'-') ? 3.0 * pTiming.unit : pTiming.unit;
            timeline.append({TimelineEvent::Kind::KeyUp, time});
        }
        pendingGap = pTiming.charGap;
        started = true;
    }
    if (started)
        timeline.append({TimelineEvent::Kind::End, time + pendingGap});
    return timeline;
}

MorseSequencer::MorseSequencer(AudioEngine* pAudio, QObject* parent)
    : QObject(parent)
    , m_audio(pAudio)
{
    connect(&m_feedTimer, &QTimer::timeout, this, &MorseSequencer::feed);
    connect(m_audio, &AudioEngine::markerReached, this, &MorseSequencer::onMarker);
}

bool MorseSequencer::play(const QString& pText, const MorseTiming& pTiming)
{
    stop();
    if (!m_audio->isRunning())
        return false;
    m_timeline = buildTimeline(pText, pTiming);
    if (m_timeline.isEmpty())
        return false;
    m_generation = m_audio->cancelScheduled();
    m_startFrame = m_audio->framesRendered() + m_audio->latencyFrames();
    m_next = 0;
    m_playing = true;
    feed();
    m_feedTimer.start(20);
    return true;
}

void MorseSequencer::stop()
{
    if (!m_playing)
        return;
    m_playing = false;
    m_feedTimer.stop();
    m_generation = m_audio->cancelScheduled();
    emit finished();
}

bool MorseSequencer::isPlaying() const
{
    return m_playing;
}

///
/// \brief MorseSequencer::feed hands the next events to the audio thread
///
void MorseSequencer::feed()
{
    const qint64 horizon = m_audio->framesRendered()
                           + static_cast<qint64>(FeedHorizon * AudioEngine::SampleRate)
                           + m_audio->latencyFrames();
    while (m_next < m_timeline.size())
    {
        const TimelineEvent& event = m_timeline.at(m_next);
        AudioEvent audioEvent;
        audioEvent.frame = m_startFrame + std::llround(event.time * AudioEngine::SampleRate);
        if (audioEvent.frame > horizon)
            break;
        audioEvent.generation = m_generation;
        switch (event.kind)
        {
        case TimelineEvent::Kind::KeyDown:
            audioEvent.type = AudioEvent::Type::KeyDown;
            audioEvent.id = static_cast<int>(KeySource::Sequencer);
            break;
        case TimelineEvent::Kind::KeyUp:
            audioEvent.type = AudioEvent::Type::KeyUp;
            audioEvent.id = static_cast<int>(KeySource::Sequencer);
            break;
        case TimelineEvent::Kind::CharStart:
            audioEvent.type = AudioEvent::Type::Marker;
            audioEvent.id = event.index;
            break;
        case TimelineEvent::Kind::End:
            audioEvent.type = AudioEvent::Type::Marker;
            audioEvent.id = EndMarker;
            break;
        }
        if (!m_audio->schedule(audioEvent))
            break; // queue full, retry on next tick
        ++m_next;
    }
    if (m_next >= m_timeline.size())
        m_feedTimer.stop();
}

void MorseSequencer::onMarker(quint32 pGeneration, int pId)
{
    if (!m_playing || pGeneration != m_generation)
        return;
    if (pId == EndMarker)
    {
        m_playing = false;
        m_feedTimer.stop();
        emit finished();
    }
    else
    {
        emit charPlayed(pId);
    }
}
