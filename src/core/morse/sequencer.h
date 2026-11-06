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
* sequencer.h is part of EasyMorse project
*
* Turns a text into Morse key events and plays it through the audio engine,
* scheduled at exact frames, without blocking the GUI.
* **************************************************************************/

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

class AudioEngine;

///
/// \brief Durations in seconds
///
struct MorseTiming
{
    double unit = 0.1;      // dot, gap between elements of a character
    double charGap = 0.3;   // between characters (3 units without Farnsworth)
    double wordGap = 0.7;   // between words (7 units without Farnsworth)

    ///
    /// \brief compute the timing from speeds in words per minute (PARIS standard)
    /// \param pWordSpeed effective speed, used with Farnsworth spacing when slower than pCharSpeed
    /// \param pCharSpeed character speed, the speed of everything without Farnsworth spacing
    ///
    static MorseTiming fromSpeed(int pWordSpeed, int pCharSpeed, bool pFarnsworth);
};

struct TimelineEvent
{
    enum class Kind
    {
        KeyDown,
        KeyUp,
        CharStart,  // index is the position of the character in the text
        End
    };
    Kind    kind;
    double  time;   // seconds since the start
    int     index = -1;
};

class MorseSequencer : public QObject
{
    Q_OBJECT

public:
    explicit MorseSequencer(AudioEngine* pAudio, QObject* parent = nullptr);

    static QList<TimelineEvent> buildTimeline(const QString& pText, const MorseTiming& pTiming);

    ///
    /// \brief play a text, replacing any playback in progress
    /// \return false if there is nothing to play or no sound output
    ///
    bool    play(const QString& pText, const MorseTiming& pTiming);
    void    stop();
    bool    isPlaying() const;

signals:
    void    charPlayed(int pIndex);
    void    finished();

private slots:
    void    feed();
    void    onMarker(quint32 pGeneration, int pId);

private:
    static constexpr int EndMarker = -1;

    AudioEngine*            m_audio;
    QList<TimelineEvent>    m_timeline;
    qsizetype               m_next = 0;
    qint64                  m_startFrame = 0;
    quint32                 m_generation = 0;
    bool                    m_playing = false;
    QTimer                  m_feedTimer;
};

#endif // SEQUENCER_H
