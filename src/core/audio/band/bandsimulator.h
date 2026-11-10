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
* bandsimulator.h is part of EasyMorse project
*
* What a CW operator hears in the headphones. Everything the antenna picks
* up (band noise, static crashes, man made noise, the station listened to
* and the stations around it) is summed in complex base band, goes through
* the receiver filter and the AGC, and is brought to the tone frequency.
* The sidetone of the own key is added afterwards, clean.
*
* Runs in the real time audio thread: no allocation, no lock.
* **************************************************************************/

#ifndef BAND_BANDSIMULATOR_H
#define BAND_BANDSIMULATOR_H

#include "cwstation.h"
#include "dsp.h"
#include "noise.h"

namespace band
{

class BandSimulator
{
public:
    enum class Parameter
    {
        NoiseLevel,     // 0..1: 1 = noise as strong as the signal in a 500 Hz filter
        Bandwidth,      // Hz
        QsbDepth,       // 0..1
        QsbRate,        // Hz: how fast the signals fade
        Flutter,        // Hz of Doppler spread, 0 = none
        QrnLevel,       // 0..1
        QrnRate,        // static crashes per minute
        ManMade,        // 0..1
        Agc,            // 0 or 1
        Count
    };
    static constexpr int MaxStations = 8;

    explicit BandSimulator(double pSampleRate, std::uint64_t pSeed = 0x9E3779B97F4A7C15ULL);

    void    setFrequency(double pHz);
    void    setGain(double pGain);
    void    set(Parameter pParameter, double pValue);

    void    setSidetone(bool pDown) { m_sidetoneDown = pDown; }
    void    setSignal(bool pDown) { m_main.setDown(pDown); }

    ///
    /// \brief play a message of a background station in its slot
    ///
    void    play(const StationScript& pScript);
    ///
    /// \return a slot whose message has just ended, or -1
    ///
    int     takeFinished();

    float   next();

    double  mainQsbGain() const { return m_main.qsbGain(m_qsbDepth); }

private:
    double          m_sampleRate;
    Random          m_random;

    double          m_frequency = 900.0;
    double          m_gain = 0.1;
    double          m_noiseSigma = 0.0;
    double          m_impulseScale;
    double          m_qsbDepth = 0.0;
    double          m_qsbRate = 0.1;
    double          m_flutter = 0.0;
    bool            m_agcOn = false;

    Qrn             m_qrn;
    ManMade         m_manMade;
    CwStation       m_main;
    CwStation       m_stations[MaxStations];
    ReceiverFilter  m_filter;
    Agc             m_agc;

    double          m_phase = 0.0;
    bool            m_sidetoneDown = false;
    double          m_sidetone = 0.0;     // raised cosine position
    double          m_edgeStep;
};

} // namespace band

#endif // BAND_BANDSIMULATOR_H
