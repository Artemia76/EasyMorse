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
* dsp.h is part of EasyMorse project
*
* Building blocks of the band simulator. The band is simulated in complex
* base band: 0 Hz is the listening frequency, a station 300 Hz above it is
* a complex tone at +300 Hz. Real time safe: no allocation, no lock.
* **************************************************************************/

#ifndef BAND_DSP_H
#define BAND_DSP_H

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <numbers>

namespace band
{

using Complex = std::complex<double>;

///
/// \brief Random numbers (xorshift64*): fast and reproducible
///
class Random
{
public:
    explicit Random(std::uint64_t pSeed = 0x9E3779B97F4A7C15ULL) : m_state(pSeed ? pSeed : 1) {}

    // Uniform in [0, 1)
    double uniform()
    {
        m_state ^= m_state >> 12;
        m_state ^= m_state << 25;
        m_state ^= m_state >> 27;
        return static_cast<double>((m_state * 0x2545F4914F6CDD1DULL) >> 11) * 0x1.0p-53;
    }
    double uniform(double pLow, double pHigh) { return pLow + (pHigh - pLow) * uniform(); }
    // Equal chances for each decade between pLow and pHigh
    double logUniform(double pLow, double pHigh) { return pLow * std::pow(pHigh / pLow, uniform()); }
    // Complex gaussian noise, variance 1 on each part (Box-Muller)
    Complex gaussian()
    {
        const double radius = std::sqrt(-2.0 * std::log(1.0 - uniform()));
        const double angle = 2.0 * std::numbers::pi * uniform();
        return {radius * std::cos(angle), radius * std::sin(angle)};
    }

private:
    std::uint64_t m_state;
};

///
/// \brief Receiver filter: 4th order Butterworth low pass on the complex base
/// band, i.e. an 8 pole band pass centered on the tone, with steep skirts
/// like the crystal or DSP filter of a CW receiver.
///
class ReceiverFilter
{
public:
    void setBandwidth(double pHz, double pSampleRate)
    {
        // Pole pairs of a 4th order Butterworth filter
        static constexpr double Q[2] = {0.54119610, 1.30656296};
        const double w0 = 2.0 * std::numbers::pi * (pHz / 2.0) / pSampleRate;
        for (int i = 0; i < 2; ++i)
        {
            const double alpha = std::sin(w0) / (2.0 * Q[i]);
            const double a0 = 1.0 + alpha;
            m_stage[i].b0 = (1.0 - std::cos(w0)) / 2.0 / a0;
            m_stage[i].b1 = (1.0 - std::cos(w0)) / a0;
            m_stage[i].a1 = -2.0 * std::cos(w0) / a0;
            m_stage[i].a2 = (1.0 - alpha) / a0;
        }
    }

    Complex process(Complex pIn)
    {
        for (Stage& stage : m_stage)
            pIn = stage.process(pIn);
        return pIn;
    }

private:
    struct Stage
    {
        double  b0 = 1.0, b1 = 0.0, a1 = 0.0, a2 = 0.0;    // b2 == b0
        Complex x1, x2, y1, y2;
        Complex process(Complex pIn)
        {
            const Complex out = b0 * (pIn + x2) + b1 * x1 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = pIn;
            y2 = y1;
            y1 = out;
            return out;
        }
    };
    Stage   m_stage[2];
};

///
/// \brief Rayleigh fading (Clarke model): sum of scattered paths with random
/// Doppler shifts. Mean power 1. A spread of 0.05 to 1 Hz gives the slow QSB,
/// 10 to 50 Hz the rasping "flutter" of auroral signals.
///
class Fading
{
public:
    static constexpr int ControlPeriod = 32;   // samples between two computed values

    void reset(Random& pRandom, double pSpreadHz, double pSampleRate)
    {
        for (Path& path : m_paths)
        {
            path.step = 2.0 * std::numbers::pi * pSpreadHz * std::cos(pRandom.uniform(0.0, 2.0 * std::numbers::pi))
                        * ControlPeriod / pSampleRate;
            path.phase = pRandom.uniform(0.0, 2.0 * std::numbers::pi);
        }
        m_from = m_to = compute();
        m_count = 0;
    }

    // Complex gain of the next sample
    Complex next()
    {
        if (m_count == ControlPeriod)
        {
            m_from = m_to;
            for (Path& path : m_paths)
                path.phase = std::fmod(path.phase + path.step, 2.0 * std::numbers::pi);
            m_to = compute();
            m_count = 0;
        }
        const double t = static_cast<double>(m_count++) / ControlPeriod;
        return m_from + (m_to - m_from) * t;
    }

private:
    static constexpr int Paths = 8;
    struct Path
    {
        double  step = 0.0;
        double  phase = 0.0;
    };

    Complex compute() const
    {
        Complex sum;
        for (const Path& path : m_paths)
            sum += std::polar(1.0, path.phase);
        return sum / std::sqrt(static_cast<double>(Paths));
    }

    Path    m_paths[Paths];
    Complex m_from{1.0, 0.0};
    Complex m_to{1.0, 0.0};
    int     m_count = 0;
};

///
/// \brief Automatic gain control: fast attack, slow release. A strong
/// signal pushes the noise down, as on a real receiver.
///
class Agc
{
public:
    static constexpr double MaxGain = 4.0;     // +12 dB on a quiet band

    void setSampleRate(double pSampleRate)
    {
        m_attack = 1.0 - std::exp(-1.0 / (0.005 * pSampleRate));
        m_release = 1.0 - std::exp(-1.0 / (0.300 * pSampleRate));
    }

    double gain(double pMagnitude)
    {
        m_envelope += (pMagnitude - m_envelope) * (pMagnitude > m_envelope ? m_attack : m_release);
        return std::min(MaxGain, 1.0 / std::max(m_envelope, 1e-6));
    }

private:
    double  m_attack = 0.0;
    double  m_release = 0.0;
    double  m_envelope = 0.0;
};

} // namespace band

#endif // BAND_DSP_H
