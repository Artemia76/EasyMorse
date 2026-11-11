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
* keyinganalyzer.cpp is part of EasyMorse project
* **************************************************************************/

#include "keyinganalyzer.h"

#include <algorithm>
#include <cmath>

#include "morse/keydecoder.h"
#include "morse/morsecodec.h"

namespace
{
    double median(QList<double> pValues)
    {
        if (pValues.isEmpty())
            return 0.0;
        std::sort(pValues.begin(), pValues.end());
        const qsizetype middle = pValues.size() / 2;
        return (pValues.size() % 2) ? pValues.at(middle) : 0.5 * (pValues.at(middle - 1) + pValues.at(middle));
    }

    ///
    /// \brief itemScore: 1 up to 5 % off, 0 from 40 % off
    ///
    double itemScore(double pError)
    {
        return std::clamp(1.0 - (std::abs(pError) - 0.05) / 0.35, 0.0, 1.0);
    }

    ///
    /// \brief charGapError: shorter than 3 dots merges the characters; up to
    /// 6 dots is only slow (Farnsworth spacing), longer splits the group
    ///
    double charGapError(double pGap, double pUnit)
    {
        if (pGap < 3.0 * pUnit)
            return (pGap - 3.0 * pUnit) / (3.0 * pUnit);
        if (pGap > 6.0 * pUnit)
            return (pGap - 6.0 * pUnit) / (6.0 * pUnit);
        return 0.0;
    }

    KeyingAnalyzer::AdviceItem item(KeyingAnalyzer::Advice pKind, double pValue = 0.0)
    {
        KeyingAnalyzer::AdviceItem advice;
        advice.kind = pKind;
        advice.value = pValue;
        return advice;
    }
}

void KeyingAnalyzer::start(const QString& pExpected, bool pIambic, double pFallbackUnitMs)
{
    m_expected = pExpected.toUpper();
    m_iambic = pIambic;
    m_fallbackUnit = pFallbackUnitMs > 0.0 ? pFallbackUnitMs : 100.0;
    m_origin = -1;
    m_downAt = -1;
    m_elements.clear();
    m_charStart = 0;
    m_chars.clear();
    m_unit = m_fallbackUnit;
    m_real.clear();
    m_ideal.clear();
    m_score = 0;
    m_advice.clear();
    m_dashDotRatio = m_signGapRatio = m_charGapRatio = m_spread = 0.0;
}

void KeyingAnalyzer::keyEdge(bool pDown, qint64 pTimeMs)
{
    if (isComplete())
        return;
    if (pDown)
    {
        if (m_downAt >= 0)
            return;
        if (m_origin < 0)
            m_origin = pTimeMs;
        m_downAt = pTimeMs;
        return;
    }
    if (m_downAt < 0)
        return;
    const qint64 down = m_downAt;
    m_downAt = -1;
    // Contact bounces: ignored, as the decoder does
    if (pTimeMs - down < KeyDecoder::MinElementMs)
        return;
    m_elements.append({down, pTimeMs});
}

void KeyingAnalyzer::charDecoded(const QString& pText, const QString& pPattern)
{
    if (isComplete() || m_charStart >= m_elements.size())
        return;
    const int index = static_cast<int>(m_chars.size());
    for (qsizetype i = m_charStart; i < m_elements.size(); ++i)
        m_elements[i].charIndex = index;
    Character character;
    character.expected = m_expected.at(index);
    character.expectedPattern = MorseCodec::encode(character.expected);
    character.sent = pText.toUpper();
    character.pattern = pPattern;
    character.start = relative(m_elements.at(m_charStart).down);
    character.end = relative(m_elements.constLast().up);
    character.correct = character.sent == QString(character.expected);
    m_chars.append(character);
    m_charStart = m_elements.size();
}

double KeyingAnalyzer::downSince() const
{
    return m_downAt >= 0 ? relative(m_downAt) : -1.0;
}

int KeyingAnalyzer::correctCount() const
{
    return static_cast<int>(std::count_if(m_chars.cbegin(), m_chars.cend(),
                                          [](const Character& pChar) { return pChar.correct; }));
}

bool KeyingAnalyzer::isDash(const Element& pElement, qsizetype pIndexInChar, double pUnit) const
{
    // As the decoder classified it once the character is decoded
    if (pElement.charIndex >= 0)
    {
        const QString& pattern = m_chars.at(pElement.charIndex).pattern;
        if (pIndexInChar < pattern.size())
            return pattern.at(pIndexInChar) == u'-';
    }
    return static_cast<double>(pElement.up - pElement.down) >= 2.0 * pUnit;
}

double KeyingAnalyzer::measureUnit() const
{
    // Elements of the decoded characters, classified as the decoder did
    QList<double> dots;
    QList<double> dashes;
    qsizetype inChar = 0;
    for (qsizetype i = 0; i < m_elements.size(); ++i)
    {
        const Element& element = m_elements.at(i);
        if (i > 0 && m_elements.at(i - 1).charIndex != element.charIndex)
            inChar = 0;
        if (element.charIndex < 0)
            break;
        const double duration = static_cast<double>(element.up - element.down);
        (isDash(element, inChar++, m_fallbackUnit) ? dashes : dots).append(duration);
    }
    // The dot of the operator is the beat: a dash is judged against three of
    // them. Groups without dots take the speed from the dashes.
    if (!dots.isEmpty())
        return median(dots);
    if (!dashes.isEmpty())
        return median(dashes) / 3.0;
    return m_fallbackUnit;
}

void KeyingAnalyzer::evaluate()
{
    m_unit = measureUnit();
    const double unit = m_unit;

    // Real keying, element by element, with the gap after each one
    m_real.clear();
    QList<double> dots, dashes, signGaps, charGaps, normalized;
    qsizetype inChar = 0;
    for (qsizetype i = 0; i < m_elements.size(); ++i)
    {
        const Element& element = m_elements.at(i);
        if (i > 0 && m_elements.at(i - 1).charIndex != element.charIndex)
            inChar = 0;
        const double duration = static_cast<double>(element.up - element.down);
        const bool dash = isDash(element, inChar++, unit);
        const double ideal = dash ? 3.0 * unit : unit;
        const bool decoded = element.charIndex >= 0;
        m_real.append({relative(element.down), relative(element.up), dash ? Kind::Dash : Kind::Dot,
                       element.charIndex, decoded && !m_iambic, (duration - ideal) / ideal});
        if (decoded)
        {
            (dash ? dashes : dots).append(duration);
            normalized.append(duration / ideal);
        }

        if (i + 1 >= m_elements.size())
            continue;
        const Element& next = m_elements.at(i + 1);
        const double gap = static_cast<double>(next.down - element.up);
        if (next.charIndex == element.charIndex)
        {
            m_real.append({relative(element.up), relative(next.down), Kind::Gap, element.charIndex,
                           decoded && !m_iambic, (gap - unit) / unit});
            if (decoded)
                signGaps.append(gap);
        }
        else
        {
            m_real.append({relative(element.up), relative(next.down), Kind::CharGap, element.charIndex,
                           decoded, charGapError(gap, unit)});
            if (decoded)
                charGaps.append(gap);
        }
    }

    buildIdeal();

    // Proportions
    m_dashDotRatio = (!dots.isEmpty() && !dashes.isEmpty()) ? median(dashes) / median(dots) : 0.0;
    m_signGapRatio = signGaps.isEmpty() ? 0.0 : median(signGaps) / unit;
    m_charGapRatio = charGaps.isEmpty() ? 0.0 : median(charGaps) / unit;
    m_spread = 0.0;
    if (normalized.size() >= 3)
    {
        double mean = 0.0;
        for (double value : std::as_const(normalized))
            mean += value;
        mean /= static_cast<double>(normalized.size());
        double variance = 0.0;
        for (double value : std::as_const(normalized))
            variance += (value - mean) * (value - mean);
        m_spread = std::sqrt(variance / static_cast<double>(normalized.size())) / mean;
    }

    // Score: characters right, then keying
    if (m_chars.isEmpty())
    {
        m_score = 0;
    }
    else
    {
        // Each kind of element or gap counts alike, and the worst one counts
        // again: good gaps must not hide dashes always too short
        double sums[4] = {};
        int counts[4] = {};
        for (const Segment& segment : std::as_const(m_real))
        {
            if (!segment.rated)
                continue;
            sums[static_cast<int>(segment.kind)] += itemScore(segment.error);
            ++counts[static_cast<int>(segment.kind)];
        }
        double mean = 0.0;
        double worst = 1.0;
        int kinds = 0;
        for (int kind = 0; kind < 4; ++kind)
        {
            if (counts[kind] == 0)
                continue;
            const double kindScore = sums[kind] / counts[kind];
            mean += kindScore;
            worst = std::min(worst, kindScore);
            ++kinds;
        }
        const double timing = kinds > 0 ? 0.5 * (mean / kinds) + 0.5 * worst : 1.0;
        const double accuracy = static_cast<double>(correctCount()) / static_cast<double>(m_chars.size());
        m_score = static_cast<int>(std::lround(100.0 * (0.4 * accuracy + 0.6 * timing)));
    }

    // Advice: the most serious faults first
    QList<std::pair<double, AdviceItem>> candidates;
    for (const Character& character : std::as_const(m_chars))
    {
        if (!character.correct)
        {
            candidates.append({10.0, {Advice::WrongCharacter, 0.0, QString(character.expected), character.pattern}});
            break;
        }
    }
    adviseFromStatistics(candidates);
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const auto& pA, const auto& pB) { return pA.first > pB.first; });
    m_advice.clear();
    for (qsizetype i = 0; i < candidates.size() && i < 2; ++i)
        m_advice.append(candidates.at(i).second);
    if (m_advice.isEmpty() && !m_chars.isEmpty())
        m_advice.append(item(Advice::WellDone));
}

void KeyingAnalyzer::adviseFromStatistics(QList<std::pair<double, AdviceItem>>& pCandidates) const
{
    if (!m_iambic)
    {
        if (m_dashDotRatio > 0.0 && m_dashDotRatio < 2.5)
            pCandidates.append({(3.0 - m_dashDotRatio) / 3.0, item(KeyingAnalyzer::Advice::DashesShort, m_dashDotRatio)});
        else if (m_dashDotRatio > 3.8)
            pCandidates.append({(m_dashDotRatio - 3.0) / 3.0, item(KeyingAnalyzer::Advice::DashesLong, m_dashDotRatio)});
        if (m_signGapRatio > 1.7)
            pCandidates.append({0.5 * (m_signGapRatio - 1.0), item(KeyingAnalyzer::Advice::SignGapsLong, m_signGapRatio)});
        else if (m_signGapRatio > 0.0 && m_signGapRatio < 0.5)
            pCandidates.append({0.5 * (1.0 - m_signGapRatio), item(KeyingAnalyzer::Advice::SignGapsShort, m_signGapRatio)});
        if (m_spread > 0.25)
            pCandidates.append({m_spread, item(KeyingAnalyzer::Advice::Irregular, m_spread)});
    }
    if (m_charGapRatio > 0.0 && m_charGapRatio < 2.2)
        pCandidates.append({(3.0 - m_charGapRatio) / 3.0, item(KeyingAnalyzer::Advice::LettersClose, m_charGapRatio)});
    else if (m_charGapRatio > 7.0)
        pCandidates.append({0.5 * (m_charGapRatio - 7.0) / 7.0, item(KeyingAnalyzer::Advice::LettersFar, m_charGapRatio)});
}

void KeyingAnalyzer::buildIdeal()
{
    // Each character keyed perfectly at the measured speed, from where the
    // real one starts; the mark after it shows where the next one should start
    m_ideal.clear();
    const double unit = m_unit;
    for (qsizetype index = 0; index < m_chars.size(); ++index)
    {
        const Character& character = m_chars.at(index);
        double time = character.start;
        const int charIndex = static_cast<int>(index);
        for (qsizetype i = 0; i < character.expectedPattern.size(); ++i)
        {
            const bool dash = character.expectedPattern.at(i) == u'-';
            const double duration = dash ? 3.0 * unit : unit;
            m_ideal.append({time, time + duration, dash ? Kind::Dash : Kind::Dot, charIndex, false, 0.0});
            time += duration;
            if (i + 1 < character.expectedPattern.size())
            {
                m_ideal.append({time, time + unit, Kind::Gap, charIndex, false, 0.0});
                time += unit;
            }
        }
        if (index + 1 < m_expected.size())
            m_ideal.append({time, time + 3.0 * unit, Kind::CharGap, charIndex, false, 0.0});
    }
}
