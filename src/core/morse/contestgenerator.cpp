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
* contestgenerator.cpp is part of EasyMorse project
* **************************************************************************/

#include "contestgenerator.h"

#include "morse/morsecodec.h"

namespace
{
    // Common prefixes; those ending with a letter get a call area digit
    const char* const Prefixes[] = {
        "F", "F", "F", "DL", "DK", "DJ", "G", "M", "EA", "I", "IK", "IZ", "ON", "PA", "OK", "OM",
        "SP", "SM", "OH", "LA", "OZ", "HA", "YU", "S5", "9A", "UA", "RA", "UR", "UT", "LZ", "YO",
        "SV", "CT", "HB9", "OE", "EI", "GM", "LY", "YL", "ES", "4X", "EU", "K", "W", "N", "AA",
        "VE", "JA", "LU", "PY", "ZS", "VK",
    };
}

ContestGenerator::ContestGenerator(double pSampleRate, std::uint64_t pSeed)
    : m_sampleRate(pSampleRate)
    , m_random(pSeed)
{
}

QString ContestGenerator::randomCallsign()
{
    constexpr int count = sizeof(Prefixes) / sizeof(Prefixes[0]);
    QString call = QString::fromLatin1(Prefixes[static_cast<int>(m_random.uniform() * count)]);
    if (!call.back().isDigit())
        call += QChar('0' + static_cast<int>(m_random.uniform(1.0, 10.0)));
    const double length = m_random.uniform();
    const int letters = length < 0.1 ? 1 : length < 0.5 ? 2 : 3;
    for (int i = 0; i < letters; ++i)
        call += QChar('A' + static_cast<int>(m_random.uniform() * 26));
    return call;
}

void ContestGenerator::newStation(Station& pStation)
{
    pStation.call = randomCallsign();
    pStation.runner = m_random.uniform() < 0.6;
    pStation.wpm = m_random.uniform(18.0, 34.0);
    pStation.jitter = m_random.uniform() < 0.3 ? 0.06 : 0.0;
    // Above or below: most in the filter, some only heard in its skirts
    const double offset = m_random.uniform() < 0.7 ? m_random.uniform(80.0, 400.0) : m_random.uniform(400.0, 1500.0);
    pStation.offsetHz = static_cast<float>((m_random.uniform() < 0.5 ? -1.0 : 1.0) * offset);
    pStation.levelDb = static_cast<float>(m_random.uniform(-18.0, 6.0));
    pStation.edgeMs = m_random.uniform() < 0.1 ? 0.7f : static_cast<float>(m_random.uniform(3.0, 7.0));
    pStation.chirpHz = m_random.uniform() < 0.1 ? static_cast<float>(m_random.uniform(30.0, 120.0)) : 0.0f;
    pStation.serial = static_cast<int>(m_random.uniform(1.0, 400.0));
    pStation.step = 0;
    pStation.remaining = pStation.runner ? static_cast<int>(m_random.uniform(6.0, 16.0))
                                         : static_cast<int>(m_random.uniform(2.0, 4.0));
}

QString ContestGenerator::serial(const Station& pStation) const
{
    QString number = QString::number(pStation.serial).rightJustified(3, '0');
    // Cut numbers, as many contesters send them: T for 0, N for 9
    if (pStation.call.size() % 2 == 0)
        number.replace('0', 'T').replace('9', 'N');
    return number;
}

QString ContestGenerator::message(Station& pStation, double& pPause)
{
    const QString& call = pStation.call;
    if (pStation.runner)
    {
        // CQ, (a caller we do not hear), report, (his report), TU and CQ again
        switch (pStation.step++ % 3)
        {
        case 0:
            pPause = m_random.uniform(1.0, 3.0);
            return m_random.uniform() < 0.5 ? QStringLiteral("CQ TEST %1 %1").arg(call)
                                            : QStringLiteral("CQ %1 TEST").arg(call);
        case 1:
            pPause = m_random.uniform(1.0, 2.5);
            return QStringLiteral("5NN %1").arg(serial(pStation));
        default:
            pPause = m_random.uniform(0.8, 2.0);
            ++pStation.serial;
            return QStringLiteral("TU %1").arg(call);
        }
    }
    // Search and pounce: call, then report, then off to another run station
    if (pStation.step++ % 2 == 0)
    {
        pPause = m_random.uniform(0.3, 2.5);
        return m_random.uniform() < 0.5 ? call : QStringLiteral("%1 %1").arg(call);
    }
    pPause = m_random.uniform(1.5, 3.0);
    return m_random.uniform() < 0.5 ? QStringLiteral("5NN %1").arg(serial(pStation))
                                    : QStringLiteral("TU 5NN %1").arg(static_cast<int>(m_random.uniform(1.0, 41.0)));
}

band::StationScript ContestGenerator::next(int pSlot)
{
    Station& station = m_slots[pSlot];
    double pause = 0.0;
    if (station.remaining-- <= 0)
    {
        newStation(station);
        // A new station: tune in at any time
        pause = m_random.uniform(0.2, 2.5);
    }
    double messagePause = 0.0;
    const QString text = message(station, messagePause);
    station.lastText = text;
    return script(pSlot, station, text, pause + messagePause);
}

band::StationScript ContestGenerator::script(int pSlot, const Station& pStation, const QString& pText, double pPause)
{
    band::StationScript script;
    script.slot = pSlot;
    script.offsetHz = pStation.offsetHz;
    script.levelDb = pStation.levelDb;
    script.edgeMs = pStation.edgeMs;
    script.chirpHz = pStation.chirpHz;

    // PARIS timing: a dot lasts 1.2 / wpm seconds
    const double dot = 1.2 / pStation.wpm * m_sampleRate;
    const auto samples = [&](double pUnits) {
        const double error = pStation.jitter > 0.0 ? m_random.uniform(-pStation.jitter, pStation.jitter) : 0.0;
        return static_cast<std::int32_t>(pUnits * dot * (1.0 + error));
    };

    int& count = script.count;
    script.durations[count++] = static_cast<std::int32_t>(pPause * m_sampleRate);
    double gap = 0.0;          // silence waiting to be written, in dots
    for (const QChar character : pText)
    {
        if (character == ' ')
        {
            gap = 7.0;
            continue;
        }
        for (const QChar element : MorseCodec::encode(character))
        {
            if (count + 2 > band::StationScript::MaxDurations)
                return script;
            if (gap > 0.0)
            {
                // Merge the silence with the one before the first key down
                if (count == 1)
                    script.durations[0] += samples(gap);
                else
                    script.durations[count++] = samples(gap);
            }
            script.durations[count++] = samples(element == '-' ? 3.0 : 1.0);
            gap = 1.0;
        }
        gap = 3.0;
    }
    return script;
}
