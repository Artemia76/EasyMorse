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
* listeningpractice.h is part of EasyMorse project
*
* Copy practice (learning by ear, Koch method): groups of the characters
* learnt are played, the operator types what they hear as it comes, and the
* copy is compared with the text sent at the end. 90 % right brings a new
* character.
* **************************************************************************/

#ifndef LISTENINGPRACTICE_H
#define LISTENINGPRACTICE_H

#include <QObject>
#include <QTimer>
#include <QVariantList>

#include "morse/kochcourse.h"

class Settings;
class Trainer;

class ListeningPractice : public QObject
{
    Q_OBJECT
    // Stops the series when the page is left
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int kochLevel READ kochLevel WRITE setKochLevel NOTIFY kochChanged)
    Q_PROPERTY(QString kochCharacters READ kochCharacters NOTIFY kochChanged)
    Q_PROPERTY(int kochMaxLevel READ kochMaxLevel CONSTANT)
    Q_PROPERTY(int groupCount READ groupCount CONSTANT)
    Q_PROPERTY(int groupLength READ groupLength CONSTANT)
    // What the operator copied, without spaces
    Q_PROPERTY(QString typed READ typed NOTIFY typedChanged)
    // Result, once the series is over
    Q_PROPERTY(QString sent READ sent NOTIFY resultChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultChanged)
    Q_PROPERTY(int correct READ correct NOTIFY resultChanged)
    Q_PROPERTY(int total READ total NOTIFY resultChanged)
    Q_PROPERTY(int extra READ extra NOTIFY resultChanged)
    Q_PROPERTY(int percent READ percent NOTIFY resultChanged)
    Q_PROPERTY(bool passed READ isPassed NOTIFY resultChanged)

public:
    enum State
    {
        Idle,
        Playing,
        Finishing,  // the sound is over, time to copy the last characters
        Done
    };
    Q_ENUM(State)

    enum CopyStatus
    {
        Correct,
        Wrong,
        Missed
    };
    Q_ENUM(CopyStatus)

    static constexpr int GroupCount = 5;
    static constexpr int GroupLength = 5;
    static constexpr int PassPercent = 90;
    // Copy time left after the sound, restarted by each character typed
    static constexpr int FinishDelayMs = 5000;

    ListeningPractice(Trainer* pTrainer, std::uint64_t pSeed, QObject* parent = nullptr);

    bool        isActive() const { return m_active; }
    void        setActive(bool pActive);
    State       state() const { return m_state; }
    int         kochLevel() const { return m_course.level(); }
    void        setKochLevel(int pLevel);
    QString     kochCharacters() const { return m_course.characters(); }
    int         kochMaxLevel() const { return KochCourse::maxLevel(); }
    int         groupCount() const { return GroupCount; }
    int         groupLength() const { return GroupLength; }
    QString     typed() const { return m_typed; }
    QString     sent() const { return m_state == Done ? m_sent : QString(); }
    QVariantList results() const { return m_results; }
    int         correct() const { return m_correct; }
    int         total() const { return static_cast<int>(m_results.size()); }
    int         extra() const { return m_extra; }
    int         percent() const;
    bool        isPassed() const { return m_state == Done && percent() >= PassPercent; }

    ///
    /// \brief start draws a new series and plays it
    /// \return false without sound output
    ///
    Q_INVOKABLE bool start();
    ///
    /// \brief stop gives up the series, nothing is scored
    ///
    Q_INVOKABLE void stop();
    ///
    /// \brief finish ends the copy and scores it
    ///
    Q_INVOKABLE void finish();
    ///
    /// \brief type adds a character copied (ignored if not in the Koch course)
    ///
    Q_INVOKABLE void type(const QString& pCharacter);
    Q_INVOKABLE void backspace();
    ///
    /// \brief replay plays the series again once it is scored
    ///
    Q_INVOKABLE void replay();

signals:
    void        activeChanged();
    void        stateChanged();
    void        kochChanged();
    void        typedChanged();
    void        resultChanged();
    ///
    /// \brief a new character joins the course after a series passed
    ///
    void        levelUp(const QString& pCharacter);

private:
    void        setState(State pState);
    void        onPlayingChanged();
    void        clearResult();

    Trainer*        m_trainer;
    Settings*       m_settings;
    KochCourse      m_course;
    QTimer          m_finishTimer;
    bool            m_active = false;
    State           m_state = Idle;
    QString         m_sent;
    QString         m_typed;
    QVariantList    m_results;
    int             m_correct = 0;
    int             m_extra = 0;
};

#endif // LISTENINGPRACTICE_H
