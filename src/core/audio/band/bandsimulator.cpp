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
* bandsimulator.cpp is part of EasyMorse project
* **************************************************************************/

#include "bandsimulator.h"

namespace band
{

namespace
{
    constexpr double ReferenceBandwidth = 500.0;
    // Noise bandwidth of the 4th order Butterworth filter, relative to its cutoff
    constexpr double NoiseBandwidthRatio = 1.026;
    constexpr double FlutterShare = 0.85;
}

BandSimulator::BandSimulator(double pSampleRate, std::uint64_t pSeed)
    : m_sampleRate(pSampleRate)
    , m_random(pSeed)
    // An impulse of amplitude 1 rings as loud as the tone through a 500 Hz filter
    , m_impulseScale(pSampleRate / ReferenceBandwidth)
    , m_edgeStep(1.0 / (0.005 * pSampleRate))
{
    m_qrn.setSampleRate(pSampleRate);
    m_manMade.reset(m_random, pSampleRate);
    m_main.setup(m_random, pSampleRate, m_qsbRate, 1.0);
    for (CwStation& station : m_stations)
        station.setup(m_random, pSampleRate, m_qsbRate, 1.0);
    m_agc.setSampleRate(pSampleRate);
    set(Parameter::Bandwidth, ReferenceBandwidth);
}

void BandSimulator::setFrequency(double pHz)
{
    m_frequency = std::clamp(pHz, 50.0, m_sampleRate / 4);
}

void BandSimulator::setGain(double pGain)
{
    m_gain = std::max(0.0, pGain);
}

void BandSimulator::set(Parameter pParameter, double pValue)
{
    switch (pParameter)
    {
    case Parameter::NoiseLevel:
        // Gaussian noise whose filtered output has the RMS of a tone of
        // amplitude pValue in the reference bandwidth
        m_noiseSigma = std::clamp(pValue, 0.0, 1.0)
                       * std::sqrt(m_sampleRate / (2.0 * NoiseBandwidthRatio * ReferenceBandwidth));
        break;
    case Parameter::Bandwidth:
        m_filter.setBandwidth(std::clamp(pValue, 50.0, 6000.0), m_sampleRate);
        break;
    case Parameter::QsbDepth:
        m_qsbDepth = std::clamp(pValue, 0.0, 1.0);
        break;
    case Parameter::QsbRate:
        m_qsbRate = std::clamp(pValue, 0.01, 5.0);
        m_main.setQsbRate(m_random, m_qsbRate);
        for (CwStation& station : m_stations)
            station.setQsbRate(m_random, m_qsbRate);
        break;
    case Parameter::Flutter:
        m_flutter = std::clamp(pValue, 0.0, 100.0);
        m_main.setFlutter(m_random, m_flutter);
        for (CwStation& station : m_stations)
            station.setFlutter(m_random, m_flutter);
        break;
    case Parameter::QrnLevel:
        m_qrn.setLevel(pValue);
        break;
    case Parameter::QrnRate:
        m_qrn.setRate(pValue);
        break;
    case Parameter::ManMade:
        m_manMade.setLevel(pValue);
        break;
    case Parameter::Agc:
        m_agcOn = pValue != 0.0;
        break;
    case Parameter::Count:
        break;
    }
}

void BandSimulator::play(const StationScript& pScript)
{
    if (pScript.slot >= 0 && pScript.slot < MaxStations)
        m_stations[pScript.slot].play(pScript);
}

int BandSimulator::takeFinished()
{
    for (int i = 0; i < MaxStations; ++i)
        if (m_stations[i].takeFinished())
            return i;
    return -1;
}

float BandSimulator::next()
{
    const double flutter = m_flutter > 0.0 ? FlutterShare : 0.0;

    // What the antenna picks up
    Complex antenna;
    if (m_noiseSigma > 0.0)
        antenna = m_noiseSigma * m_random.gaussian();
    antenna += m_qrn.next(m_random, m_impulseScale);
    antenna += m_manMade.next(m_random, m_impulseScale);
    antenna += m_main.next(m_qsbDepth, flutter);
    for (CwStation& station : m_stations)
        antenna += station.next(m_qsbDepth, flutter);

    // Receiver: filter, AGC, then up to the tone frequency
    const Complex filtered = m_filter.process(antenna);
    const double agc = m_agc.gain(std::abs(filtered));
    m_phase = std::fmod(m_phase + 2.0 * std::numbers::pi * m_frequency / m_sampleRate, 2.0 * std::numbers::pi);
    const double received = (m_agcOn ? agc : 1.0) * (filtered * std::polar(1.0, m_phase)).real();

    // Own sidetone, clean; the receiver is partly muted while sending
    m_sidetone = m_sidetoneDown ? std::min(1.0, m_sidetone + m_edgeStep) : std::max(0.0, m_sidetone - m_edgeStep);
    const double sidetone = 0.5 * (1.0 - std::cos(std::numbers::pi * m_sidetone));

    return static_cast<float>(m_gain * (received * (1.0 - 0.5 * sidetone) + sidetone * std::cos(m_phase)));
}

} // namespace band
