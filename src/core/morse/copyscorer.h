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
* copyscorer.h is part of EasyMorse project
*
* Copy practice: compares the text sent with the text the operator copied.
* Spaces and case are ignored, and the two texts are aligned (edit distance)
* so that a character missed does not make all the next ones wrong.
* **************************************************************************/

#ifndef COPYSCORER_H
#define COPYSCORER_H

#include <QList>
#include <QString>

class CopyScorer
{
public:
    enum class Status
    {
        Correct,
        Wrong,      // another character was copied in its place
        Missed      // nothing was copied for it
    };

    struct Item
    {
        QChar   expected;
        QChar   copied;     // null when missed
        Status  status;
    };

    ///
    /// \brief score aligns the copied text with the sent one
    ///
    static CopyScorer score(const QString& pSent, const QString& pCopied);

    ///
    /// \brief one item per character sent (spaces removed), in order
    ///
    const QList<Item>& items() const { return m_items; }
    int     correct() const { return m_correct; }
    int     total() const { return static_cast<int>(m_items.size()); }
    ///
    /// \brief characters copied that match no character sent
    ///
    int     extra() const { return m_extra; }
    ///
    /// \brief share of the characters sent copied right, 0 to 1
    ///
    double  accuracy() const;

    ///
    /// \brief normalize removes the spaces and puts the text in upper case
    ///
    static QString normalize(const QString& pText);

private:
    QList<Item> m_items;
    int         m_correct = 0;
    int         m_extra = 0;
};

#endif // COPYSCORER_H
