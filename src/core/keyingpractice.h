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
* keyingpractice.h is part of EasyMorse project
*
* Keying practice: a group of Koch characters is shown, the operator keys
* it, and the keying is compared with the ideal one (KeyingAnalyzer) for the
* timeline and the appreciation of the practice page.
* **************************************************************************/

#ifndef KEYINGPRACTICE_H
#define KEYINGPRACTICE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>

#include "morse/keyinganalyzer.h"
#include "morse/kochcourse.h"

class Settings;
class Trainer;

class KeyingPractice : public QObject
{
    Q_OBJECT
    // Listens to the key only while the practice page is shown
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    // Group to key, and how many of its characters are decoded
    Q_PROPERTY(QString group READ group NOTIFY groupChanged)
    Q_PROPERTY(int position READ position NOTIFY groupChanged)
    Q_PROPERTY(int kochLevel READ kochLevel WRITE setKochLevel NOTIFY kochChanged)
    Q_PROPERTY(QString kochCharacters READ kochCharacters NOTIFY kochChanged)
    Q_PROPERTY(int kochMaxLevel READ kochMaxLevel CONSTANT)
    // Analysis of the group being keyed (live) or of the last one keyed
    Q_PROPERTY(bool live READ isLive NOTIFY analysisChanged)
    Q_PROPERTY(QString analysisGroup READ analysisGroup NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList characters READ characters NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList realSegments READ realSegments NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList idealSegments READ idealSegments NOTIFY analysisChanged)
    Q_PROPERTY(double downSince READ downSince NOTIFY analysisChanged)
    Q_PROPERTY(double unitMs READ unitMs NOTIFY analysisChanged)
    Q_PROPERTY(bool scored READ isScored NOTIFY analysisChanged)
    Q_PROPERTY(int score READ score NOTIFY analysisChanged)
    Q_PROPERTY(Verdict verdict READ verdict NOTIFY analysisChanged)
    Q_PROPERTY(QStringList advice READ advice NOTIFY analysisChanged)

public:
    enum Verdict
    {
        NoVerdict,
        Excellent,
        Good,
        NeedsWork
    };
    Q_ENUM(Verdict)

    static constexpr int GroupLength = 5;
    static constexpr int ExcellentScore = 85;
    static constexpr int GoodScore = 65;

    explicit KeyingPractice(Trainer* pTrainer, QObject* parent = nullptr);

    bool        isActive() const { return m_active; }
    void        setActive(bool pActive);
    QString     group() const { return m_group; }
    int         position() const { return m_position; }
    int         kochLevel() const { return m_course.level(); }
    void        setKochLevel(int pLevel);
    QString     kochCharacters() const { return m_course.characters(); }
    int         kochMaxLevel() const { return KochCourse::maxLevel(); }

    bool        isLive() const { return m_live; }
    QString     analysisGroup() const { return m_analyzer.expected(); }
    QVariantList characters() const { return m_characters; }
    QVariantList realSegments() const { return m_realSegments; }
    QVariantList idealSegments() const { return m_idealSegments; }
    double      downSince() const { return m_analyzer.downSince(); }
    double      unitMs() const { return m_analyzer.unitMs(); }
    bool        isScored() const { return m_scored; }
    int         score() const { return m_analyzer.score(); }
    Verdict     verdict() const;
    QStringList advice() const;

    ///
    /// \brief elapsed ms since the first key press of the group analysed, -1 before it
    ///
    Q_INVOKABLE double elapsed() const;
    Q_INVOKABLE void newGroup();
    ///
    /// \brief restart keys the same group again
    ///
    Q_INVOKABLE void restart();
    ///
    /// \brief listen plays the group as a model
    ///
    Q_INVOKABLE void listen();

signals:
    void        activeChanged();
    void        groupChanged();
    void        kochChanged();
    void        analysisChanged();
    ///
    /// \brief a new character joins the course after good groups
    ///
    void        levelUp(const QString& pCharacter);

private:
    void        onKeyEdge(bool pDown, qint64 pTimeMs);
    void        onCharDecoded(const QString& pText, const QString& pPattern);
    void        refresh();
    void        finish();
    double      fallbackUnit() const;

    Trainer*        m_trainer;
    Settings*       m_settings;
    KochCourse      m_course;
    KeyingAnalyzer  m_analyzer;
    bool            m_active = false;
    QString         m_group;
    int             m_position = 0;
    bool            m_started = false;  // the group is being keyed
    bool            m_live = false;
    bool            m_scored = false;
    QVariantList    m_characters;
    QVariantList    m_realSegments;
    QVariantList    m_idealSegments;
};

#endif // KEYINGPRACTICE_H
