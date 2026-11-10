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
* radiochannel.cpp is part of EasyMorse project
* **************************************************************************/

#include "radiochannel.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
    constexpr double EdgeSeconds = 0.005;           // rise and fall time of the tone
    constexpr double ReferenceBandwidth = 2400.0;   // noise level 1 = signal level in this width
    constexpr double FadeMinSeconds = 2.0;
    constexpr double FadeMaxSeconds = 10.0;
}

RadioChannel::RadioChannel(double pSampleRate, std::uint64_t pSeed)
    : m_sampleRate(pSampleRate)
    , m_state(pSeed ? pSeed : 1)
    , m_edgeStep(1.0 / (EdgeSeconds * pSampleRate))
    // Noise level 1: noise as strong as the tone (RMS) in the reference width
    , m_noiseScale(std::sqrt(pSampleRate / (std::numbers::pi * ReferenceBandwidth)) / std::numbers::sqrt2)
{
    updateFilter();
    updateFade();
}

void RadioChannel::setFrequency(double pHz)
{
    m_frequency = std::clamp(pHz, 50.0, m_sampleRate / 4);
    updateFilter();
}

void RadioChannel::setBandwidth(double pHz)
{
    m_bandwidth = std::clamp(pHz, 50.0, m_sampleRate / 4);
    updateFilter();
}

void RadioChannel::setNoiseLevel(double pLevel)
{
    m_noiseLevel = std::clamp(pLevel, 0.0, 1.0);
}

void RadioChannel::setQsbDepth(double pDepth)
{
    m_qsbDepth = std::clamp(pDepth, 0.0, 1.0);
}

void RadioChannel::setGain(double pGain)
{
    m_gain = std::max(0.0, pGain);
}

float RadioChannel::next()
{
    // Tone: the sidetone stays clean, the received signal fades
    const double sidetone = m_sidetone.next(m_edgeStep);
    const double signal = m_signal.next(m_edgeStep) * qsbGain();
    double value = std::max(sidetone, signal) * std::sin(m_phase);
    m_phase += 2.0 * std::numbers::pi * m_frequency / m_sampleRate;
    if (m_phase >= 2.0 * std::numbers::pi)
        m_phase -= 2.0 * std::numbers::pi;

    // Band noise. A white noise of variance 1 through the band pass filter
    // keeps a variance of pi * bandwidth / sample rate: narrowing the
    // filter removes noise, as on a real receiver.
    if (m_noiseLevel > 0.0)
    {
        const double white = (random() * 2.0 - 1.0) * std::sqrt(3.0);
        const double band = m_b0 * white + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = white;
        m_y2 = m_y1;
        m_y1 = band;
        value += m_noiseLevel * band * m_noiseScale;
    }

    // Fading
    m_fadeTime += 1.0;
    if (m_fadeTime >= m_fadeLength)
        updateFade();
    else
        m_fade = m_fadeFrom + (m_fadeTo - m_fadeFrom) * 0.5 * (1.0 - std::cos(std::numbers::pi * m_fadeTime / m_fadeLength));

    return static_cast<float>(value * m_gain);
}

double RadioChannel::Envelope::next(double pStep)
{
    position = down ? std::min(1.0, position + pStep) : std::max(0.0, position - pStep);
    return 0.5 * (1.0 - std::cos(std::numbers::pi * position));
}

double RadioChannel::random()
{
    // xorshift64*: fast and good enough for audio noise
    m_state ^= m_state >> 12;
    m_state ^= m_state << 25;
    m_state ^= m_state >> 27;
    return static_cast<double>((m_state * 0x2545F4914F6CDD1DULL) >> 11) * 0x1.0p-53;
}

void RadioChannel::updateFilter()
{
    // Band pass of the Audio EQ Cookbook (R. Bristow-Johnson), 0 dB at the center
    const double w0 = 2.0 * std::numbers::pi * m_frequency / m_sampleRate;
    const double alpha = std::sin(w0) * m_bandwidth / (2.0 * m_frequency);
    const double a0 = 1.0 + alpha;
    m_b0 = alpha / a0;
    m_b2 = -alpha / a0;
    m_a1 = -2.0 * std::cos(w0) / a0;
    m_a2 = (1.0 - alpha) / a0;
}

void RadioChannel::updateFade()
{
    m_fadeFrom = m_fade = m_fadeTo;
    m_fadeTo = random();
    m_fadeTime = 0.0;
    m_fadeLength = (FadeMinSeconds + random() * (FadeMaxSeconds - FadeMinSeconds)) * m_sampleRate;
}
