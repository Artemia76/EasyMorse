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
* kochcourse.cpp is part of EasyMorse project
* **************************************************************************/

#include "kochcourse.h"

#include <algorithm>

QString KochCourse::order()
{
    return QStringLiteral("KMURESNAPTLWI.JZ=FOY,VG5/Q92H38B?47C1D60X");
}

KochCourse::KochCourse(std::uint64_t pSeed)
    : m_random(pSeed)
{
}

void KochCourse::setLevel(int pLevel)
{
    m_level = std::clamp(pLevel, MinLevel, maxLevel());
    m_results.clear();
}

QString KochCourse::nextGroup(int pLength)
{
    const QString learnt = characters();
    QString group;
    QChar previous;
    for (int i = 0; i < pLength; ++i)
    {
        QChar next;
        // The same character twice in a row tells little about the keying
        do
        {
            if (m_random.uniform() < 1.0 / 3.0)
                next = newest();
            else
                next = learnt.at(static_cast<qsizetype>(m_random.uniform() * learnt.size()));
        } while (next == previous && learnt.size() > 1);
        group += next;
        previous = next;
    }
    return group;
}

bool KochCourse::recordResult(int pCorrect, int pTotal, int pScore)
{
    if (pTotal <= 0)
        return false;
    m_results.append({static_cast<double>(pCorrect) / pTotal, pScore});
    while (m_results.size() > GroupsToPass)
        m_results.removeFirst();
    if (m_results.size() < GroupsToPass || m_level >= maxLevel())
        return false;
    const bool passed = std::all_of(m_results.cbegin(), m_results.cend(), [](const Result& pResult) {
        return pResult.accuracy >= PassAccuracy && pResult.score >= PassScore;
    });
    if (!passed)
        return false;
    setLevel(m_level + 1);
    return true;
}
