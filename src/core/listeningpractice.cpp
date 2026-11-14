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
* listeningpractice.cpp is part of EasyMorse project
* **************************************************************************/

#include "listeningpractice.h"

#include <algorithm>
#include <cmath>

#include <QStringList>
#include <QVariantMap>

#include "morse/copyscorer.h"
#include "settings/settings.h"
#include "trainer.h"

ListeningPractice::ListeningPractice(Trainer* pTrainer, std::uint64_t pSeed, QObject* parent)
    : QObject(parent)
    , m_trainer(pTrainer)
    , m_settings(pTrainer->settings())
    , m_course(pSeed)
{
    m_course.setLevel(m_settings->listeningKochLevel());
    connect(m_settings, &Settings::listeningKochLevelChanged, this, [this] {
        if (m_settings->listeningKochLevel() == m_course.level())
            return;
        m_course.setLevel(m_settings->listeningKochLevel());
        emit kochChanged();
    });
    m_finishTimer.setSingleShot(true);
    m_finishTimer.setInterval(FinishDelayMs);
    connect(&m_finishTimer, &QTimer::timeout, this, &ListeningPractice::finish);
    connect(m_trainer, &Trainer::playingChanged, this, &ListeningPractice::onPlayingChanged);
}

void ListeningPractice::setActive(bool pActive)
{
    if (pActive == m_active)
        return;
    m_active = pActive;
    if (!m_active)
        stop();
    emit activeChanged();
}

void ListeningPractice::setKochLevel(int pLevel)
{
    m_settings->setListeningKochLevel(std::clamp(pLevel, KochCourse::MinLevel, KochCourse::maxLevel()));
}

int ListeningPractice::percent() const
{
    return m_results.isEmpty() ? 0 : static_cast<int>(std::floor(100.0 * m_correct / m_results.size()));
}

bool ListeningPractice::start()
{
    QStringList groups;
    for (int i = 0; i < GroupCount; ++i)
        groups.append(m_course.nextGroup(GroupLength, true));    // pure chance, repeats included
    const QString sent = groups.join(' ');
    if (!m_trainer->play(sent, true))
        return false;
    m_sent = sent;
    m_finishTimer.stop();
    clearResult();
    if (!m_typed.isEmpty())
    {
        m_typed.clear();
        emit typedChanged();
    }
    setState(Playing);
    return true;
}

void ListeningPractice::stop()
{
    // The state first: the end of the sound must not look like the end of the series
    const bool ours = m_state != Idle;
    if (m_state == Playing || m_state == Finishing)
        setState(Idle);
    m_finishTimer.stop();
    if (ours && m_trainer->isPlaying())
        m_trainer->stop();
}

void ListeningPractice::finish()
{
    if (m_state != Playing && m_state != Finishing)
        return;
    const CopyScorer score = CopyScorer::score(m_sent, m_typed);
    m_results.clear();
    for (const CopyScorer::Item& item : score.items())
    {
        const CopyStatus status = item.status == CopyScorer::Status::Correct ? Correct
                                  : item.status == CopyScorer::Status::Wrong ? Wrong
                                                                             : Missed;
        m_results.append(QVariantMap{
            {QStringLiteral("expected"), QString(item.expected)},
            {QStringLiteral("copied"), item.copied.isNull() ? QString() : QString(item.copied)},
            {QStringLiteral("status"), static_cast<int>(status)}
        });
    }
    m_correct = score.correct();
    m_extra = score.extra();
    setState(Done);
    m_finishTimer.stop();
    // All copied before the end of the sound
    if (m_trainer->isPlaying())
        m_trainer->stop();

    if (isPassed() && m_course.level() < KochCourse::maxLevel())
    {
        setKochLevel(m_course.level() + 1);
        emit levelUp(QString(m_course.newest()));
    }
}

void ListeningPractice::type(const QString& pCharacter)
{
    if (m_state != Playing && m_state != Finishing)
        return;
    const QString character = pCharacter.trimmed().toUpper();
    // Only the characters of the course can be sent
    if (character.size() != 1 || !KochCourse::order().contains(character.at(0)))
        return;
    m_typed += character;
    emit typedChanged();
    if (m_typed.size() >= GroupCount * GroupLength)
        finish();
    else if (m_state == Finishing)
        m_finishTimer.start();
}

void ListeningPractice::backspace()
{
    if ((m_state != Playing && m_state != Finishing) || m_typed.isEmpty())
        return;
    m_typed.chop(1);
    emit typedChanged();
    if (m_state == Finishing)
        m_finishTimer.start();
}

void ListeningPractice::replay()
{
    if (m_state == Done)
        m_trainer->play(m_sent, true);
}

void ListeningPractice::setState(State pState)
{
    if (pState == m_state)
        return;
    const bool wasDone = m_state == Done;
    m_state = pState;
    emit stateChanged();
    // The text sent shows only once the series is scored
    if (wasDone || m_state == Done)
        emit resultChanged();
}

void ListeningPractice::onPlayingChanged()
{
    if (m_state == Playing && !m_trainer->isPlaying())
    {
        setState(Finishing);
        m_finishTimer.start();
    }
}

void ListeningPractice::clearResult()
{
    m_results.clear();
    m_correct = 0;
    m_extra = 0;
}
