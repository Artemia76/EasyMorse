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
* keyinganalyzer.h is part of EasyMorse project
*
* Compares a keyed group of characters with the ideal keying of the same
* characters. The ideal uses the speed of the operator, measured on the
* group itself (the median of the dots): what is judged is the proportions
* (dash = 3 dots, gap inside a character = 1 dot, gap between characters =
* 3 dots), not the speed. The characters are cut where the KeyDecoder cut them.
* **************************************************************************/

#ifndef KEYINGANALYZER_H
#define KEYINGANALYZER_H

#include <QList>
#include <QString>

class KeyingAnalyzer
{
public:
    // Keying errors (relative to the ideal duration) shown as good / fair
    static constexpr double GoodError = 0.2;
    static constexpr double FairError = 0.4;

    enum class Kind
    {
        Dot,
        Dash,
        Gap,        // between the elements of a character
        CharGap     // between characters
    };

    struct Segment
    {
        double  start;      // ms from the first key press of the group
        double  end;
        Kind    kind;
        int     charIndex;  // character of the group, -1 if not decoded yet
        bool    rated;      // error is meaningful
        double  error;      // (duration - ideal) / ideal
    };

    struct Character
    {
        QChar   expected;
        QString expectedPattern;
        QString sent;       // decoded character, empty if unknown
        QString pattern;    // dots and dashes received
        double  start;
        double  end;
        bool    correct;
    };

    enum class Advice
    {
        WrongCharacter,     // text: expected, detail: pattern sent
        DashesShort,        // value: dash / dot ratio
        DashesLong,
        SignGapsLong,       // value: gap inside characters, in dots
        SignGapsShort,
        LettersClose,       // value: gap between characters, in dots
        LettersFar,
        Irregular,          // value: spread of the elements, relative
        WellDone
    };

    struct AdviceItem
    {
        Advice  kind;
        double  value = 0.0;
        QString text;
        QString detail;
    };

    ///
    /// \brief start a new group
    /// \param pExpected characters to send
    /// \param pIambic the keyer makes the elements: only the gaps between
    /// characters and the characters themselves are judged
    /// \param pFallbackUnitMs dot duration used until a character is decoded
    ///
    void    start(const QString& pExpected, bool pIambic, double pFallbackUnitMs);
    void    keyEdge(bool pDown, qint64 pTimeMs);
    ///
    /// \brief charDecoded closes the character made of the elements keyed so far
    ///
    void    charDecoded(const QString& pText, const QString& pPattern);

    bool    isStarted() const { return m_origin >= 0; }
    bool    isComplete() const { return m_chars.size() >= m_expected.size(); }
    QString expected() const { return m_expected; }
    qsizetype decodedCount() const { return m_chars.size(); }
    ///
    /// \brief time of the first key press, -1 before it
    ///
    qint64  origin() const { return m_origin; }
    ///
    /// \brief start of the key press in progress (ms from origin), -1 if the key is up
    ///
    double  downSince() const;

    ///
    /// \brief evaluate measures the group keyed so far; the getters below
    /// return its results
    ///
    void    evaluate();

    double                  unitMs() const { return m_unit; }
    const QList<Segment>&   realSegments() const { return m_real; }
    const QList<Segment>&   idealSegments() const { return m_ideal; }
    const QList<Character>& characters() const { return m_chars; }
    int                     correctCount() const;
    ///
    /// \brief score from 0 to 100: characters right and keying proportions
    ///
    int                     score() const { return m_score; }
    ///
    /// \brief the two most useful pieces of advice
    ///
    const QList<AdviceItem>& advice() const { return m_advice; }
    double                  dashDotRatio() const { return m_dashDotRatio; }
    double                  signGapRatio() const { return m_signGapRatio; }
    double                  charGapRatio() const { return m_charGapRatio; }

private:
    struct Element
    {
        qint64  down;
        qint64  up;
        int     charIndex = -1;
    };

    double  relative(qint64 pTimeMs) const { return static_cast<double>(pTimeMs - m_origin); }
    bool    isDash(const Element& pElement, qsizetype pIndexInChar, double pUnit) const;
    double  measureUnit() const;
    void    buildIdeal();
    void    adviseFromStatistics(QList<std::pair<double, AdviceItem>>& pCandidates) const;

    QString         m_expected;
    bool            m_iambic = false;
    double          m_fallbackUnit = 100.0;
    qint64          m_origin = -1;
    qint64          m_downAt = -1;
    QList<Element>  m_elements;
    qsizetype       m_charStart = 0;    // first element of the character being keyed
    QList<Character> m_chars;

    double              m_unit = 100.0;
    QList<Segment>      m_real;
    QList<Segment>      m_ideal;
    int                 m_score = 0;
    QList<AdviceItem>   m_advice;
    double              m_dashDotRatio = 0.0;
    double              m_signGapRatio = 0.0;
    double              m_charGapRatio = 0.0;
    double              m_spread = 0.0;
};

#endif // KEYINGANALYZER_H
