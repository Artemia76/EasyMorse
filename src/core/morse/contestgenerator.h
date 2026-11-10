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
* contestgenerator.h is part of EasyMorse project
*
* Background stations of a busy band during a contest: stations calling
* CQ and giving reports, others answering them, each with its own call,
* speed, offset, strength and keying. They make QRM: they do not answer
* the operator.
* **************************************************************************/

#ifndef CONTESTGENERATOR_H
#define CONTESTGENERATOR_H

#include <QString>

#include "audio/band/bandsimulator.h"

class ContestGenerator
{
public:
    explicit ContestGenerator(double pSampleRate, std::uint64_t pSeed);

    ///
    /// \brief next message of the station in a slot, with the pause before it
    ///
    band::StationScript next(int pSlot);
    ///
    /// \brief text of the last message of a slot (tests, debugging)
    ///
    QString lastText(int pSlot) const { return m_slots[pSlot].lastText; }

    QString randomCallsign();

private:
    struct Station
    {
        QString call;
        bool    runner = true;      // calls CQ, or answers the runners
        double  wpm = 25.0;
        double  jitter = 0.0;       // timing errors of a hand sent signal
        float   offsetHz = 0.0f;
        float   levelDb = 0.0f;         // -18..+6 dB from the station listened to
        float   edgeMs = 5.0f;
        float   chirpHz = 0.0f;
        int     serial = 1;
        int     step = 0;
        int     remaining = 0;      // messages before another station takes the slot
        QString lastText;
    };

    void            newStation(Station& pStation);
    QString         message(Station& pStation, double& pPause);
    QString         serial(const Station& pStation) const;
    band::StationScript script(int pSlot, const Station& pStation, const QString& pText, double pPause);

    double          m_sampleRate;
    band::Random    m_random;
    Station         m_slots[band::BandSimulator::MaxStations];
};

#endif // CONTESTGENERATOR_H
