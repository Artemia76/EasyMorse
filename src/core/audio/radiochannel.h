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
* radiochannel.h is part of EasyMorse project
*
* What a CW operator hears in the headphones: the sidetone of the own key,
* the signal received (played text, remote stations) that fades in and out
* (QSB), and the band noise passed through the receiver filter.
*
* Runs in the real time audio thread: no allocation, no lock.
* **************************************************************************/

#ifndef RADIOCHANNEL_H
#define RADIOCHANNEL_H

#include <cstdint>

class RadioChannel
{
public:
    explicit RadioChannel(double pSampleRate, std::uint64_t pSeed = 0x9E3779B97F4A7C15ULL);

    void    setFrequency(double pHz);
    ///
    /// \brief setBandwidth width of the receiver filter, centered on the tone
    ///
    void    setBandwidth(double pHz);
    ///
    /// \brief setNoiseLevel 0 (no noise) to 1 (noise as loud as the signal in a 2.4 kHz filter)
    ///
    void    setNoiseLevel(double pLevel);
    ///
    /// \brief setQsbDepth 0 (steady signal) to 1 (the signal may fade out completely)
    ///
    void    setQsbDepth(double pDepth);
    void    setGain(double pGain);

    void    setSidetone(bool pDown) { m_sidetone.down = pDown; }
    void    setSignal(bool pDown) { m_signal.down = pDown; }

    float   next();

    ///
    /// \brief qsbGain current attenuation of the received signal (1 = no fading)
    ///
    double  qsbGain() const { return 1.0 - m_qsbDepth * m_fade; }

private:
    // Raised cosine edges: no key clicks
    struct Envelope
    {
        bool    down = false;
        double  position = 0.0;     // 0 (silent) .. 1 (full)
        double  next(double pStep);
    };

    double  random();               // uniform in [0, 1)
    void    updateFilter();
    void    updateFade();

    double          m_sampleRate;
    std::uint64_t   m_state;

    double          m_frequency = 900.0;
    double          m_bandwidth = 500.0;
    double          m_noiseLevel = 0.0;
    double          m_qsbDepth = 0.0;
    double          m_gain = 0.1;

    // Tone
    double          m_phase = 0.0;
    double          m_edgeStep;
    Envelope        m_sidetone;
    Envelope        m_signal;

    // Band pass filter of the noise (biquad, direct form I)
    double          m_noiseScale;
    double          m_b0 = 0.0, m_b2 = 0.0, m_a1 = 0.0, m_a2 = 0.0;
    double          m_x1 = 0.0, m_x2 = 0.0, m_y1 = 0.0, m_y2 = 0.0;

    // Fading: smooth moves between random depths, of random durations
    double          m_fade = 0.0;           // 0 (no fading) .. 1 (deepest)
    double          m_fadeFrom = 0.0;
    double          m_fadeTo = 0.0;
    double          m_fadeTime = 0.0;       // samples since the move started
    double          m_fadeLength = 1.0;     // samples
};

#endif // RADIOCHANNEL_H
