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
* cwstation.h is part of EasyMorse project
*
* A Morse station heard on the band: its own offset, strength, fading and
* keying defects (chirp, clicks).
* **************************************************************************/

#ifndef BAND_CWSTATION_H
#define BAND_CWSTATION_H

#include <cstdint>

#include "dsp.h"

namespace band
{

///
/// \brief Message sent by a background station, prepared in the GUI thread.
/// durations[0] is the silence before the message, then key down, key up...
///
struct StationScript
{
    static constexpr int MaxDurations = 400;

    int             slot = 0;
    float           offsetHz = 0.0f;        // from the listening frequency
    float           levelDb = 0.0f;         // 0 dB: as strong as the main station
    float           edgeMs = 5.0f;          // rise and fall time, short = key clicks
    float           chirpHz = 0.0f;         // frequency jump at key down
    int             count = 0;              // 0 silences the slot
    std::int32_t    durations[MaxDurations] = {};  // in samples
};

class CwStation
{
public:
    void setup(Random& pRandom, double pSampleRate, double pQsbRateHz, double pFlutterHz)
    {
        m_sampleRate = pSampleRate;
        m_chirpDecay = std::exp(-1.0 / (0.020 * pSampleRate));
        setEdge(5.0);
        m_qsb.reset(pRandom, pQsbRateHz, pSampleRate);
        m_flutter.reset(pRandom, pFlutterHz, pSampleRate);
    }
    void setQsbRate(Random& pRandom, double pHz) { m_qsb.reset(pRandom, pHz, m_sampleRate); }
    void setFlutter(Random& pRandom, double pHz) { m_flutter.reset(pRandom, pHz, m_sampleRate); }

    void setOffset(double pHz) { m_offset = pHz; }
    void setLevel(double pDb) { m_amplitude = std::pow(10.0, pDb / 20.0); }
    void setEdge(double pMs) { m_edgeStep = 1.0 / (std::max(0.2, pMs) * 0.001 * m_sampleRate); }
    void setChirp(double pHz) { m_chirpHz = pHz; }

    // Keyed from outside (played text, remote stations)
    void setDown(bool pDown) { m_down = pDown; }

    // Keyed by a script (background stations)
    void play(const StationScript& pScript)
    {
        m_script = pScript;
        m_index = 0;
        m_remaining = pScript.count > 0 ? pScript.durations[0] : 0;
        m_playing = pScript.count > 0;
        m_down = false;
        setOffset(pScript.offsetHz);
        setLevel(pScript.levelDb);
        setEdge(pScript.edgeMs);
        setChirp(pScript.chirpHz);
    }
    bool isPlaying() const { return m_playing; }
    ///
    /// \brief finished true once, when the script has just ended
    ///
    bool takeFinished()
    {
        const bool finished = m_finished;
        m_finished = false;
        return finished;
    }

    ///
    /// \param pQsbDepth 0..1, depth of the slow fading
    /// \param pFlutter 0..1, share of the signal smeared by the flutter
    ///
    Complex next(double pQsbDepth, double pFlutter)
    {
        if (m_playing)
            advanceScript();

        // Raised cosine edges
        const bool wasSilent = m_position == 0.0;
        m_position = m_down ? std::min(1.0, m_position + m_edgeStep) : std::max(0.0, m_position - m_edgeStep);
        if (m_down && wasSilent)
            m_chirp = m_chirpHz;
        m_chirp *= m_chirpDecay;

        m_phase = std::fmod(m_phase + 2.0 * std::numbers::pi * (m_offset + m_chirp) / m_sampleRate,
                            2.0 * std::numbers::pi);
        const Complex qsb = m_lastQsb = m_qsb.next();
        const Complex flutter = m_flutter.next();
        if (m_position == 0.0)
            return {};

        const double envelope = 0.5 * (1.0 - std::cos(std::numbers::pi * m_position));
        const double fade = (1.0 - pQsbDepth) + pQsbDepth * std::min(std::abs(qsb), 2.0);
        const Complex smear = (1.0 - pFlutter) + pFlutter * flutter;
        return m_amplitude * envelope * fade * smear * std::polar(1.0, m_phase);
    }

    // Slow fading of the station, for the tests
    double qsbGain(double pQsbDepth) const { return (1.0 - pQsbDepth) + pQsbDepth * std::min(std::abs(m_lastQsb), 2.0); }

private:
    void advanceScript()
    {
        while (m_remaining <= 0)
        {
            if (++m_index >= m_script.count)
            {
                m_playing = false;
                m_finished = true;
                m_down = false;
                return;
            }
            m_remaining = m_script.durations[m_index];
            m_down = (m_index % 2) == 1;
        }
        --m_remaining;
    }

    double          m_sampleRate = 48000.0;
    double          m_offset = 0.0;
    double          m_amplitude = 1.0;
    double          m_edgeStep = 0.004;
    double          m_chirpHz = 0.0;
    double          m_chirp = 0.0;
    double          m_chirpDecay = 1.0;

    bool            m_down = false;
    double          m_position = 0.0;
    double          m_phase = 0.0;
    Fading          m_qsb;
    Fading          m_flutter;
    Complex         m_lastQsb{1.0, 0.0};

    StationScript   m_script;
    int             m_index = 0;
    std::int32_t    m_remaining = 0;
    bool            m_playing = false;
    bool            m_finished = false;
};

} // namespace band

#endif // BAND_CWSTATION_H
