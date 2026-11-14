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

    // cost[i][j]: edits to turn the characters sent from i into the ones copied from j
    std::vector<int> cost(static_cast<size_t>((rows + 1) * (columns + 1)));
    auto at = [&](qsizetype i, qsizetype j) -> int& {
        return cost[static_cast<size_t>(i * (columns + 1) + j)];
    };
    for (qsizetype i = 0; i <= rows; ++i)
        at(i, columns) = static_cast<int>(rows - i);
    for (qsizetype j = 0; j <= columns; ++j)
        at(rows, j) = static_cast<int>(columns - j);
    for (qsizetype i = rows - 1; i >= 0; --i)
    {
        for (qsizetype j = columns - 1; j >= 0; --j)
        {
            const int diagonal = at(i + 1, j + 1) + (sent[i] == copied[j] ? 0 : 1);
            at(i, j) = std::min({diagonal, at(i + 1, j) + 1, at(i, j + 1) + 1});
        }
    }

    // Forward from the start: the copy is matched with the beginning of the
    // series (a copy stopped early misses the end), and a character copied in
    // place of the one sent is preferred to a missed one followed by an extra one
    CopyScorer result;
    qsizetype i = 0;
    qsizetype j = 0;
    while (i < rows || j < columns)
    {
        if (i < rows && j < columns && at(i, j) == at(i + 1, j + 1) + (sent[i] == copied[j] ? 0 : 1))
        {
            const bool right = sent[i] == copied[j];
            result.m_items.append({sent[i], copied[j], right ? Status::Correct : Status::Wrong});
            result.m_correct += right ? 1 : 0;
            ++i;
            ++j;
        }
        else if (i < rows && at(i, j) == at(i + 1, j) + 1)
        {
            result.m_items.append({sent[i], QChar(), Status::Missed});
            ++i;
        }
        else
        {
            ++result.m_extra;
            ++j;
        }
    }
    return result;
}
