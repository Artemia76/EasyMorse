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
* keydecoder.h is part of EasyMorse project
*
* Streaming Morse decoder: turns key down/up edges into characters as soon
* as the silence after a character is long enough, and continuously adapts
* to the sender speed.
* **************************************************************************/

#ifndef KEYDECODER_H
#define KEYDECODER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include "keyer/monotonicclock.h"

class KeyDecoder : public QObject
{
    Q_OBJECT

public:
    // Shorter key presses are contact bounces and are ignored
    static constexpr qint64 MinElementMs = 10;

    ///
    /// \brief How fast the speed estimate follows the sender. Faster follows
    /// speed changes sooner, slower is less disturbed by clumsy keying. Large
    /// speed changes are always followed at once.
    ///
    enum class Reactivity
    {
        Fast,
        Normal,
        Stable
    };
    Q_ENUM(Reactivity)

    explicit KeyDecoder(QObject* parent = nullptr);

    ///
    /// \brief reset estimates to a nominal speed (dot = 1200 / wpm ms)
    ///
    void    setSpeed(double pWpm);
    ///
    /// \brief restore estimates saved from a previous session
    ///
    void    setEstimates(double pDotMs, double pDashMs, double pGapMs);
    double  dotMs() const { return m_dot; }
    double  dashMs() const { return m_dash; }
    double  gapMs() const { return m_gap; }
    double  charGapMs() const { return m_charGap; }
    double  wordGapMs() const { return m_wordGap; }
    double  speed() const;

    ///
    /// \brief when off, the speed stays locked to the last setSpeed()
    ///
    void    setAutoAdapt(bool pAutoAdapt) { m_autoAdapt = pAutoAdapt; }
    bool    autoAdapt() const { return m_autoAdapt; }

    void        setReactivity(Reactivity pReactivity) { m_reactivity = pReactivity; }
    Reactivity  reactivity() const { return m_reactivity; }

    ///
    /// \brief when on (default), an internal timer ends characters and words
    /// in real time; tests turn it off and call advanceTo() themselves
    ///
    void    setAutoTimer(bool pAutoTimer);

    // Thresholds derived from the estimates, in ms
    double  dotDashThreshold() const;
    double  charGapThreshold() const;
    double  wordGapThreshold() const;

    ///
    /// \brief current time on the clock used by the keyers, see MonotonicClock
    ///
    qint64  now() const { return MonotonicClock::nowMs(); }

public slots:
    ///
    /// \brief keyEdge records a key change
    /// \param pDown key pressed
    /// \param pTimeMs time of the change on a monotonic clock
    ///
    void    keyEdge(bool pDown, qint64 pTimeMs);
    void    keyEdgeNow(bool pDown) { keyEdge(pDown, now()); }
    ///
    /// \brief advanceTo ends the current character / word if the silence is long enough
    ///
    void    advanceTo(qint64 pTimeMs);
    ///
    /// \brief clear forgets the character being keyed
    ///
    void    clear();

signals:
    void    symbolProgress(const QString& pPattern);
    ///
    /// \param pText decoded character, empty if the pattern is unknown
    /// \param pPattern dots and dashes received
    ///
    void    charDecoded(const QString& pText, const QString& pPattern);
    void    wordGap();
    void    speedChanged(double pWpm);

private:
    ///
    /// \return true if the decoder relocked on a new speed
    ///
    bool    learnElement(double pDuration);
    void    learnGap(double pDuration);
    void    learnSilence(double pDuration);
    void    armTimer();
    double  unit() const;

    QTimer          m_gapTimer;
    bool            m_autoTimer = true;
    bool            m_autoAdapt = true;
    Reactivity      m_reactivity = Reactivity::Normal;
    bool            m_down = false;
    bool            m_wordPending = false;
    qint64          m_lastDown = 0;
    qint64          m_lastUp = -1;
    QString         m_pattern;
    QList<double>   m_patternDurations; // elements of the character being keyed
    QList<double>   m_history;      // last element durations
    double          m_dot = 100.0;
    double          m_dash = 300.0;
    double          m_gap = 100.0;  // silence between elements of a character
    QList<double>   m_silences;     // last silences between characters or words
    double          m_charGap = 300.0;
    double          m_wordGap = 700.0;
    double          m_lastSpeed = 0.0;
};

#endif // KEYDECODER_H
