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
* kochcourse.h is part of EasyMorse project
*
* Koch method: the characters are learnt one at a time, in a fixed order.
* Groups of random characters are drawn from the ones learnt, the newest
* one more often, and a new character comes once the groups are sent well.
* **************************************************************************/

#ifndef KOCHCOURSE_H
#define KOCHCOURSE_H

#include <QList>
#include <QString>

#include "audio/band/dsp.h"

class KochCourse
{
public:
    static constexpr int MinLevel = 2;
    // Groups judged together before a new character comes
    static constexpr int GroupsToPass = 5;
    static constexpr double PassAccuracy = 0.9;
    static constexpr int PassScore = 70;

    ///
    /// \brief order of the characters (the order of LCWO)
    ///
    static QString order();
    static int maxLevel() { return static_cast<int>(order().size()); }

    explicit KochCourse(std::uint64_t pSeed);

    ///
    /// \brief setLevel sets the number of characters learnt and forgets the last results
    ///
    void    setLevel(int pLevel);
    int     level() const { return m_level; }
    QString characters() const { return order().left(m_level); }
    QChar   newest() const { return order().at(m_level - 1); }

    ///
    /// \brief nextGroup draws a group from the characters learnt; the newest
    /// one comes about a third of the time
    ///
    QString nextGroup(int pLength = 5);

    ///
    /// \brief recordResult remembers how a group was sent
    /// \return true if a new character was added
    ///
    bool    recordResult(int pCorrect, int pTotal, int pScore);

private:
    struct Result
    {
        double  accuracy;
        int     score;
    };

    band::Random    m_random;
    int             m_level = MinLevel;
    QList<Result>   m_results;
};

#endif // KOCHCOURSE_H
