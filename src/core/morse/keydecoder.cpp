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
* keydecoder.cpp is part of EasyMorse project
*
* Streaming, self adapting Morse decoder
* **************************************************************************/

#include "keydecoder.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "morse/morsecodec.h"

namespace
{
    ///
    /// \brief How fast the estimates follow the sender, see KeyDecoder::Reactivity
    ///
    struct Adaptation
    {
        qsizetype   historySize;    // recent elements or silences considered
        double      smoothing;      // share of a new estimate taken at each element
        double      driftRate;      // same, when only one kind of element was received
        bool        median;         // median of each group (robust) or mean (moves faster)
    };

    constexpr Adaptation Adaptations[] = {
        {12, 1.0, 0.30, false},     // Fast: the behaviour of version 1.0
        {20, 0.35, 0.15, true},     // Normal
        {32, 0.15, 0.08, true},     // Stable
    };

    // A new estimate this far from the current one is a new sender or a
    // real change of speed: it is taken at once instead of being smoothed
    constexpr double JumpRatio = 1.5;
    // Recent elements checked for a change of speed: when both their dots and
    // their dashes (at least 2 of each) are JumpRatio faster or slower than
    // expected, the older history is dropped to lock on the new speed. One
    // clumsy element cannot move both groups.
    constexpr qsizetype RecentElements = 4;
    // Until this many elements are known (start, after a relock) the decoder
    // adapts as fast as possible to lock on the sender, smoothing comes after
    constexpr qsizetype WarmUpElements = 8;
    constexpr qsizetype MinRecentPerGroup = 2;
    // Dashes are nominally 3 dots, accept from 2
    constexpr double MinDashDotRatio = 2.0;
    // Word gaps are nominally 7/3 of character gaps
    constexpr double MinWordCharRatio = 1.6;

    double median(QList<double> pValues)
    {
        std::sort(pValues.begin(), pValues.end());
        const qsizetype middle = pValues.size() / 2;
        return (pValues.size() % 2) ? pValues[middle] : (pValues[middle - 1] + pValues[middle]) / 2.0;
    }

    ///
    /// \brief splitTwo finds two groups of values (two-means clustering)
    /// \param pLow typical short value: median (a single clumsy value barely
    /// moves it) or mean of the short group
    /// \param pHigh typical long value
    /// \param pSmallest number of values in the smaller group
    /// \return true if both groups exist and are at least pMinRatio apart
    ///
    bool splitTwo(const QList<double>& pValues, double pMinRatio, double& pLow, double& pHigh,
                  qsizetype& pSmallest, bool pMedian = true)
    {
        if (pValues.isEmpty())
            return false;
        const auto [minIt, maxIt] = std::minmax_element(pValues.cbegin(), pValues.cend());
        double low = *minIt;
        double high = *maxIt;
        if (high < pMinRatio * low)
            return false;
        for (int iteration = 0; iteration < 8; ++iteration)
        {
            const double threshold = (low + high) / 2.0;
            double sumLow = 0.0, sumHigh = 0.0;
            int countLow = 0, countHigh = 0;
            for (double value : pValues)
            {
                if (value < threshold) { sumLow += value; ++countLow; }
                else { sumHigh += value; ++countHigh; }
            }
            low = sumLow / countLow;
            high = sumHigh / countHigh;
        }
        if (high < pMinRatio * low)
            return false;
        const double threshold = (low + high) / 2.0;
        QList<double> lows, highs;
        for (double value : pValues)
            (value < threshold ? lows : highs).append(value);
        pLow = pMedian ? median(lows) : low;
        pHigh = pMedian ? median(highs) : high;
        pSmallest = qMin(lows.size(), highs.size());
        return true;
    }

    // A group of elements needs this many values before a large change is
    // trusted: a single clumsy element must not be taken as a new speed.
    // Word gaps are rare, a single one is enough for the silences.
    constexpr qsizetype MinElementsForJump = 3;
    constexpr qsizetype MinSilencesForJump = 1;

    ///
    /// \brief follow moves both estimates toward new values: at once when the
    /// change is large and trusted, smoothly otherwise
    ///
    void follow(double& pShort, double& pLong, double pNewShort, double pNewLong,
                bool pTrusted, double pSmoothing)
    {
        const bool jump = pTrusted
                       && (pNewShort > JumpRatio * pShort || pNewShort * JumpRatio < pShort
                           || pNewLong > JumpRatio * pLong || pNewLong * JumpRatio < pLong);
        const double rate = jump ? 1.0 : pSmoothing;
        pShort += rate * (pNewShort - pShort);
        pLong += rate * (pNewLong - pLong);
    }

    ///
    /// \brief drift moves the estimate closest to the value (by ratio) toward
    /// it, and the other one so that their nominal ratio is kept
    ///
    void drift(double pValue, double& pShort, double& pLong, double pNominalRatio, double pRate)
    {
        if (std::abs(std::log(pValue / pShort)) <= std::abs(std::log(pValue / pLong)))
        {
            pShort += pRate * (pValue - pShort);
            pLong += pRate * (pNominalRatio * pShort - pLong);
        }
        else
        {
            pLong += pRate * (pValue - pLong);
            pShort += pRate * (pLong / pNominalRatio - pShort);
        }
    }
}

KeyDecoder::KeyDecoder(QObject* parent)
    : QObject(parent)
{
    m_gapTimer.setSingleShot(true);
    connect(&m_gapTimer, &QTimer::timeout, this, [this] { advanceTo(now()); });
    setSpeed(12.0);
}

void KeyDecoder::setSpeed(double pWpm)
{
    m_dot = 1200.0 / qMax(1.0, pWpm);
    m_dash = 3.0 * m_dot;
    m_gap = m_dot;
    m_charGap = 3.0 * m_dot;
    m_wordGap = 7.0 * m_dot;
    m_history.clear();
    m_silences.clear();
    m_lastSpeed = speed();
    emit speedChanged(m_lastSpeed);
}

void KeyDecoder::setEstimates(double pDotMs, double pDashMs, double pGapMs)
{
    if (pDotMs < MinElementMs || pDashMs < MinDashDotRatio * pDotMs || pGapMs <= 0.0)
        return;
    m_dot = pDotMs;
    m_dash = pDashMs;
    m_gap = pGapMs;
    m_charGap = 3.0 * unit();
    m_wordGap = 7.0 * unit();
    m_history.clear();
    m_silences.clear();
    m_lastSpeed = speed();
    emit speedChanged(m_lastSpeed);
}

void KeyDecoder::setAutoTimer(bool pAutoTimer)
{
    m_autoTimer = pAutoTimer;
    armTimer();
}

double KeyDecoder::unit() const
{
    return (m_dot + m_dash / 3.0) / 2.0;
}

double KeyDecoder::speed() const
{
    return 1200.0 / unit();
}

double KeyDecoder::dotDashThreshold() const
{
    return (m_dot + m_dash) / 2.0;
}

double KeyDecoder::charGapThreshold() const
{
    // Between the 1 unit element gap and the 3 units character gap. The gap
    // estimate is kept consistent with the element speed, which may just
    // have jumped.
    return 2.0 * std::clamp(m_gap, 0.6 * unit(), 1.4 * unit());
}

double KeyDecoder::wordGapThreshold() const
{
    // 5 units for a 3 units character gap. Taken from the character gap,
    // which Farnsworth spacing stretches, and not from the word gaps, which
    // pauses in the conversation make longer.
    return qMax(charGapThreshold() + unit(), m_charGap * 5.0 / 3.0);
}

void KeyDecoder::keyEdge(bool pDown, qint64 pTimeMs)
{
    if (pDown == m_down)
        return;
    if (pDown)
    {
        if (m_lastUp >= 0)
        {
            // The previous character or word may be over
            advanceTo(pTimeMs);
            const double silence = static_cast<double>(pTimeMs - m_lastUp);
            if (!m_pattern.isEmpty())
                learnGap(silence);
            else
                learnSilence(silence);
        }
        m_down = true;
        m_lastDown = pTimeMs;
        m_gapTimer.stop();
        return;
    }

    m_down = false;
    const double duration = static_cast<double>(pTimeMs - m_lastDown);
    if (duration < MinElementMs)
    {
        armTimer();
        return;
    }
    m_patternDurations.append(duration);
    if (learnElement(duration))
    {
        // New speed: the elements already keyed are classified again
        m_pattern.clear();
        for (double element : std::as_const(m_patternDurations))
            m_pattern += (element < dotDashThreshold()) ? u'.' : u'-';
    }
    else
    {
        m_pattern += (duration < dotDashThreshold()) ? u'.' : u'-';
    }
    m_lastUp = pTimeMs;
    emit symbolProgress(m_pattern);
    const double wpm = speed();
    if (qAbs(wpm - m_lastSpeed) >= 0.5)
    {
        m_lastSpeed = wpm;
        emit speedChanged(wpm);
    }
    armTimer();
}

void KeyDecoder::advanceTo(qint64 pTimeMs)
{
    if (m_down || m_lastUp < 0)
        return;
    const double silence = static_cast<double>(pTimeMs - m_lastUp);
    if (!m_pattern.isEmpty() && silence >= charGapThreshold())
    {
        const QString pattern = m_pattern;
        m_pattern.clear();
        m_patternDurations.clear();
        m_wordPending = true;
        emit charDecoded(MorseCodec::decode(pattern), pattern);
    }
    if (m_wordPending && silence >= wordGapThreshold())
    {
        m_wordPending = false;
        emit wordGap();
    }
    armTimer();
}

void KeyDecoder::clear()
{
    m_pattern.clear();
    m_patternDurations.clear();
    m_wordPending = false;
    m_gapTimer.stop();
}

///
/// \brief KeyDecoder::learnElement updates the dot and dash durations
/// \details When the recent elements contain both dots and dashes, a two
/// cluster split gives both durations at once, so a single dot + dash pair is
/// enough to lock on a new speed. Otherwise the matching estimate drifts and
/// the other one follows with the nominal 1:3 ratio.
///
bool KeyDecoder::learnElement(double pDuration)
{
    if (!m_autoAdapt)
        return false;
    // A very long press is the key held down, not a dash
    if (pDuration > 5.0 * m_dash)
        return false;
    const Adaptation& adaptation = Adaptations[static_cast<int>(m_reactivity)];
    m_history.append(pDuration);
    while (m_history.size() > adaptation.historySize)
        m_history.removeFirst();

    // The recent elements show another speed: older ones would only slow
    // down the new lock
    bool relock = false;
    const QList<double> recent = m_history.mid(qMax(0, m_history.size() - RecentElements));
    double recentDot, recentDash;
    qsizetype recentSmallest;
    if (splitTwo(recent, MinDashDotRatio, recentDot, recentDash, recentSmallest)
        && recentSmallest >= MinRecentPerGroup)
    {
        const bool faster = recentDot * JumpRatio < m_dot && recentDash * JumpRatio < m_dash;
        const bool slower = recentDot > JumpRatio * m_dot && recentDash > JumpRatio * m_dash;
        if (faster || slower)
        {
            relock = true;
            m_history = recent;
        }
    }

    const double previousUnit = unit();
    const bool warmingUp = relock || m_history.size() < WarmUpElements;
    const Adaptation& fast = Adaptations[static_cast<int>(Reactivity::Fast)];
    double low, high;
    qsizetype smallest;
    if (splitTwo(m_history, MinDashDotRatio, low, high, smallest, adaptation.median))
    {
        follow(m_dot, m_dash, low, high, smallest >= MinElementsForJump || warmingUp || !adaptation.median,
               warmingUp ? fast.smoothing : adaptation.smoothing);
    }
    else
    {
        // Only one kind of element lately
        drift(pDuration, m_dot, m_dash, 3.0, warmingUp ? fast.driftRate : adaptation.driftRate);
    }
    if (relock)
    {
        // Silences learnt before the lock were measured with the old speed,
        // some of them wrongly: start again from the nominal spacing
        m_gap = unit();
        m_charGap = 3.0 * unit();
        m_wordGap = 7.0 * unit();
        m_silences.clear();
        return true;
    }
    // The same sender spaces characters and words in proportion
    const double scale = unit() / previousUnit;
    m_charGap *= scale;
    m_wordGap *= scale;
    for (double& silence : m_silences)
        silence *= scale;
    return false;
}

void KeyDecoder::learnGap(double pDuration)
{
    if (!m_autoAdapt)
        return;
    m_gap += Adaptations[static_cast<int>(m_reactivity)].driftRate * (pDuration - m_gap);
    // Keep the gap consistent with the element speed
    m_gap = std::clamp(m_gap, 0.6 * unit(), 1.4 * unit());
}

///
/// \brief KeyDecoder::learnSilence updates the character and word gaps
/// \param pDuration a silence longer than the gap between elements
///
void KeyDecoder::learnSilence(double pDuration)
{
    if (!m_autoAdapt)
        return;
    // Pause in the conversation, not a word gap
    if (pDuration > qMax(5.0 * m_wordGap, 3000.0))
        return;
    const Adaptation& adaptation = Adaptations[static_cast<int>(m_reactivity)];
    m_silences.append(pDuration);
    while (m_silences.size() > adaptation.historySize)
        m_silences.removeFirst();
    double low, high;
    qsizetype smallest;
    if (splitTwo(m_silences, MinWordCharRatio, low, high, smallest))
        follow(m_charGap, m_wordGap, low, high, smallest >= MinSilencesForJump, adaptation.smoothing);
    else
        drift(pDuration, m_charGap, m_wordGap, 7.0 / 3.0, adaptation.driftRate);
}

///
/// \brief KeyDecoder::armTimer schedules the next character or word end check
///
void KeyDecoder::armTimer()
{
    if (!m_autoTimer || m_down || m_lastUp < 0)
    {
        m_gapTimer.stop();
        return;
    }
    double deadline;
    if (!m_pattern.isEmpty())
        deadline = m_lastUp + charGapThreshold();
    else if (m_wordPending)
        deadline = m_lastUp + wordGapThreshold();
    else
    {
        m_gapTimer.stop();
        return;
    }
    m_gapTimer.start(qMax(0, static_cast<int>(std::ceil(deadline - now()))));
}
