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
* noise.h is part of EasyMorse project
*
* Noises of the HF bands that are not a gentle hiss: static crashes of
* thunderstorms (QRN) and man made noise (mains buzz, stray carrier).
* They are wide band impulses: the receiver filter shapes them.
* **************************************************************************/

#ifndef BAND_NOISE_H
#define BAND_NOISE_H

#include "dsp.h"

namespace band
{

///
/// \brief Atmospheric noise: a constant crackle of distant lightning, and
/// crashes of nearer strikes, of random length and strength.
///
class Qrn
{
public:
    void setSampleRate(double pSampleRate) { m_sampleRate = pSampleRate; }
    // 0..1: strength of the crackle and of the crashes
    void setLevel(double pLevel) { m_level = std::clamp(pLevel, 0.0, 1.0); }
    // Crashes per minute
    void setRate(double pPerMinute) { m_rate = std::max(0.0, pPerMinute); }

    // pScale turns an impulse amplitude into an audible level through the filter
    Complex next(Random& pRandom, double pScale)
    {
        if (m_level <= 0.0)
            return {};
        Complex out;
        // Distant lightning: sparse small impulses all the time
        if (pRandom.uniform() < 0.002)
            out += impulse(pRandom, 0.4 * m_level * pScale);
        // Crash: dense impulses for a while
        if (m_remaining > 0)
        {
            --m_remaining;
            if (pRandom.uniform() < 0.01)
                out += impulse(pRandom, m_crash * pScale);
        }
        else if (pRandom.uniform() < m_rate / 60.0 / m_sampleRate)
        {
            m_remaining = static_cast<int>(pRandom.logUniform(0.02, 0.8) * m_sampleRate);
            m_crash = m_level * pRandom.logUniform(0.5, 8.0);
        }
        return out;
    }

private:
    static Complex impulse(Random& pRandom, double pAmplitude)
    {
        return {pAmplitude * pRandom.uniform(-1.0, 1.0), pAmplitude * pRandom.uniform(-1.0, 1.0)};
    }

    double  m_sampleRate = 48000.0;
    double  m_level = 0.0;
    double  m_rate = 0.0;
    int     m_remaining = 0;
    double  m_crash = 0.0;
};

///
/// \brief Man made noise: the 100 Hz buzz of an arcing power line or of a
/// rectifier (impulses at twice the 50 Hz mains), and the carrier of a
/// switching power supply drifting slowly near the listening frequency.
///
class ManMade
{
public:
    void reset(Random& pRandom, double pSampleRate)
    {
        m_sampleRate = pSampleRate;
        m_period = pSampleRate / 100.0;
        m_untilPulse = m_period;
        m_carrierOffset = pRandom.uniform(-350.0, 350.0);
        m_drift = 0.0;
    }
    // 0..1
    void setLevel(double pLevel) { m_level = std::clamp(pLevel, 0.0, 1.0); }

    Complex next(Random& pRandom, double pScale)
    {
        if (m_level <= 0.0)
            return {};
        Complex out;
        // Buzz: one impulse per half mains cycle, with some jitter
        if (--m_untilPulse <= 0.0)
        {
            m_untilPulse += m_period + pRandom.uniform(-0.0002, 0.0002) * m_sampleRate;
            out += 0.6 * m_level * pScale * pRandom.uniform(0.8, 1.2);
        }
        // Stray carrier, drifting by +-15 Hz over 40 s
        m_drift += 2.0 * std::numbers::pi / (40.0 * m_sampleRate);
        const double frequency = m_carrierOffset + 15.0 * std::sin(m_drift);
        m_carrierPhase = std::fmod(m_carrierPhase + 2.0 * std::numbers::pi * frequency / m_sampleRate,
                                   2.0 * std::numbers::pi);
        out += std::polar(0.15 * m_level, m_carrierPhase);
        return out;
    }

private:
    double  m_sampleRate = 48000.0;
    double  m_level = 0.0;
    double  m_period = 480.0;
    double  m_untilPulse = 480.0;
    double  m_carrierOffset = 0.0;
    double  m_carrierPhase = 0.0;
    double  m_drift = 0.0;
};

} // namespace band

#endif // BAND_NOISE_H
