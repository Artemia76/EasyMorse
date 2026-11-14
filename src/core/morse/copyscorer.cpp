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
* copyscorer.cpp is part of EasyMorse project
* **************************************************************************/

#include "copyscorer.h"

#include <algorithm>
#include <vector>

QString CopyScorer::normalize(const QString& pText)
{
    QString text;
    text.reserve(pText.size());
    for (QChar character : pText)
    {
        if (!character.isSpace())
            text += character.toUpper();
    }
    return text;
}

double CopyScorer::accuracy() const
{
    return m_items.isEmpty() ? 0.0 : static_cast<double>(m_correct) / m_items.size();
}

CopyScorer CopyScorer::score(const QString& pSent, const QString& pCopied)
{
    const QString sent = normalize(pSent);
    const QString copied = normalize(pCopied);
    const qsizetype rows = sent.size();
    const qsizetype columns = copied.size();

    // cost[i][j]: edits to turn the first i characters sent into the first j copied
    std::vector<int> cost(static_cast<size_t>((rows + 1) * (columns + 1)));
    auto at = [&](qsizetype i, qsizetype j) -> int& {
        return cost[static_cast<size_t>(i * (columns + 1) + j)];
    };
    for (qsizetype i = 0; i <= rows; ++i)
        at(i, 0) = static_cast<int>(i);
    for (qsizetype j = 0; j <= columns; ++j)
        at(0, j) = static_cast<int>(j);
    for (qsizetype i = 1; i <= rows; ++i)
    {
        for (qsizetype j = 1; j <= columns; ++j)
        {
            const int diagonal = at(i - 1, j - 1) + (sent[i - 1] == copied[j - 1] ? 0 : 1);
            at(i, j) = std::min({diagonal, at(i - 1, j) + 1, at(i, j - 1) + 1});
        }
    }

    // Back from the end; a character copied in place of the one sent is
    // preferred to a missed one followed by an extra one
    CopyScorer result;
    QList<Item> reversed;
    qsizetype i = rows;
    qsizetype j = columns;
    while (i > 0 || j > 0)
    {
        if (i > 0 && j > 0
            && at(i, j) == at(i - 1, j - 1) + (sent[i - 1] == copied[j - 1] ? 0 : 1))
        {
            const bool right = sent[i - 1] == copied[j - 1];
            reversed.append({sent[i - 1], copied[j - 1], right ? Status::Correct : Status::Wrong});
            result.m_correct += right ? 1 : 0;
            --i;
            --j;
        }
        else if (i > 0 && at(i, j) == at(i - 1, j) + 1)
        {
            reversed.append({sent[i - 1], QChar(), Status::Missed});
            --i;
        }
        else
        {
            ++result.m_extra;
            --j;
        }
    }
    std::reverse(reversed.begin(), reversed.end());
    result.m_items = reversed;
    return result;
}
