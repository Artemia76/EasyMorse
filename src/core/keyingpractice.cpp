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
* keyingpractice.cpp is part of EasyMorse project
* **************************************************************************/

#include "keyingpractice.h"

#include <algorithm>

#include <QLocale>
#include <QRandomGenerator>
#include <QVariantMap>

#include "keyer/monotonicclock.h"
#include "morse/morsecodec.h"
#include "settings/settings.h"
#include "trainer.h"

namespace
{
    QString kindName(KeyingAnalyzer::Kind pKind)
    {
        switch (pKind)
        {
        case KeyingAnalyzer::Kind::Dot:     return QStringLiteral("dot");
        case KeyingAnalyzer::Kind::Dash:    return QStringLiteral("dash");
        case KeyingAnalyzer::Kind::Gap:     return QStringLiteral("gap");
        case KeyingAnalyzer::Kind::CharGap: return QStringLiteral("charGap");
        }
        return {};
    }

    QVariantList toVariant(const QList<KeyingAnalyzer::Segment>& pSegments)
    {
        QVariantList list;
        list.reserve(pSegments.size());
        for (const KeyingAnalyzer::Segment& segment : pSegments)
        {
            list.append(QVariantMap{
                {"start", segment.start},
                {"end", segment.end},
                {"kind", kindName(segment.kind)},
                {"charIndex", segment.charIndex},
                {"rated", segment.rated},
                {"error", segment.error}
            });
        }
        return list;
    }

    // Same signs as Theme.pattern() in the interface
    QString signs(const QString& pPattern)
    {
        QString text = pPattern;
        text.replace(u'.', QChar(0x00B7)).replace(u'-', QChar(0x2212));
        return text;
    }

    QString dots(double pValue)
    {
        return QLocale().toString(pValue, 'f', 1);
    }
}

KeyingPractice::KeyingPractice(Trainer* pTrainer, QObject* parent)
    : QObject(parent)
    , m_trainer(pTrainer)
    , m_settings(pTrainer->settings())
    , m_course(QRandomGenerator::global()->generate64())
{
    m_course.setLevel(m_settings->kochLevel());
    connect(m_settings, &Settings::kochLevelChanged, this, [this] {
        if (m_settings->kochLevel() == m_course.level())
            return;
        m_course.setLevel(m_settings->kochLevel());
        emit kochChanged();
        newGroup();
    });
    connect(m_trainer, &Trainer::localKeyEdge, this, &KeyingPractice::onKeyEdge);
    connect(m_trainer, &Trainer::charDecoded, this, &KeyingPractice::onCharDecoded);
}

void KeyingPractice::setActive(bool pActive)
{
    if (pActive == m_active)
        return;
    m_active = pActive;
    if (m_active)
    {
        if (m_group.isEmpty())
            newGroup();
        else
            restart();
    }
    emit activeChanged();
}

void KeyingPractice::setKochLevel(int pLevel)
{
    m_settings->setKochLevel(std::clamp(pLevel, KochCourse::MinLevel, KochCourse::maxLevel()));
}

KeyingPractice::Verdict KeyingPractice::verdict() const
{
    if (!m_scored)
        return NoVerdict;
    if (score() >= ExcellentScore)
        return Excellent;
    return score() >= GoodScore ? Good : NeedsWork;
}

QStringList KeyingPractice::advice() const
{
    QStringList list;
    for (const KeyingAnalyzer::AdviceItem& item : m_analyzer.advice())
    {
        switch (item.kind)
        {
        case KeyingAnalyzer::Advice::WrongCharacter:
            list.append(tr("%1 was keyed %2 instead of %3.")
                        .arg(item.text, item.detail.isEmpty() ? QStringLiteral("?") : signs(item.detail),
                             signs(MorseCodec::encode(item.text.at(0)))));
            break;
        case KeyingAnalyzer::Advice::DashesShort:
            list.append(tr("Dashes too short: %1 dots instead of 3. Hold the key longer.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::DashesLong:
            list.append(tr("Dashes too long: %1 dots instead of 3.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::SignGapsLong:
            list.append(tr("Pauses inside the letters too long (%1 dots instead of 1): the letters break up.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::SignGapsShort:
            list.append(tr("Pauses inside the letters too short (%1 dots instead of 1): the signs run together.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::LettersClose:
            list.append(tr("Letters too close together (%1 dots instead of 3): they merge.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::LettersFar:
            list.append(tr("Letters too far apart (%1 dots): the group splits into words.").arg(dots(item.value)));
            break;
        case KeyingAnalyzer::Advice::Irregular:
            list.append(tr("Irregular rhythm: give every dot, and every dash, the same length."));
            break;
        case KeyingAnalyzer::Advice::WellDone:
            list.append(tr("Regular rhythm and right proportions, well done."));
            break;
        }
    }
    return list;
}

double KeyingPractice::elapsed() const
{
    if (!m_analyzer.isStarted())
        return -1.0;
    return static_cast<double>(MonotonicClock::nowMs() - m_analyzer.origin());
}

QString KeyingPractice::pattern(const QString& pCharacter) const
{
    return pCharacter.isEmpty() ? QString() : MorseCodec::encode(pCharacter.at(0));
}

void KeyingPractice::newGroup()
{
    m_group = m_course.nextGroup(GroupLength);
    restart();
}

void KeyingPractice::restart()
{
    m_position = 0;
    m_started = false;
    // A character begun elsewhere must not become the first of the group
    m_trainer->decoder()->clear();
    if (m_live)
    {
        // Group left half keyed: nothing to show
        m_live = false;
        m_analyzer.start(QString(), false, fallbackUnit());
        refresh();
    }
    emit groupChanged();
}

void KeyingPractice::listen()
{
    m_trainer->play(m_group);
}

double KeyingPractice::fallbackUnit() const
{
    // The iambic keyer keys at the set speed; a straight key at the speed learnt by the decoder
    if (m_settings->keyerMode() == 1)
        return 1200.0 / qMax(1, m_settings->charSpeed());
    return m_trainer->decoder()->dotMs();
}

void KeyingPractice::onKeyEdge(bool pDown, qint64 pTimeMs)
{
    if (!m_active || m_group.isEmpty())
        return;
    if (!m_started)
    {
        if (!pDown)
            return;
        m_analyzer.start(m_group, m_settings->keyerMode() == 1, fallbackUnit());
        m_started = true;
        m_live = true;
        m_scored = false;
    }
    m_analyzer.keyEdge(pDown, pTimeMs);
    if (pDown)
        emit analysisChanged();
    else
        refresh();
}

void KeyingPractice::onCharDecoded(const QString& pText, const QString& pPattern)
{
    if (!m_active || !m_started)
        return;
    m_analyzer.charDecoded(pText, pPattern);
    m_position = static_cast<int>(m_analyzer.decodedCount());
    emit groupChanged();
    if (m_analyzer.isComplete())
        finish();
    else
        refresh();
}

void KeyingPractice::refresh()
{
    m_analyzer.evaluate();
    m_realSegments = toVariant(m_analyzer.realSegments());
    m_idealSegments = toVariant(m_analyzer.idealSegments());
    m_characters.clear();
    for (const KeyingAnalyzer::Character& character : m_analyzer.characters())
    {
        m_characters.append(QVariantMap{
            {"expected", QString(character.expected)},
            {"expectedPattern", character.expectedPattern},
            {"sent", character.sent},
            {"pattern", character.pattern},
            {"correct", character.correct},
            {"start", character.start},
            {"end", character.end}
        });
    }
    emit analysisChanged();
}

void KeyingPractice::finish()
{
    m_started = false;
    m_live = false;
    m_scored = true;
    refresh();
    if (m_course.recordResult(m_analyzer.correctCount(), static_cast<int>(m_analyzer.expected().size()), score()))
    {
        m_settings->setKochLevel(m_course.level());
        emit kochChanged();
        emit levelUp(QString(m_course.newest()));
    }
    // The next group is shown at once; the result stays until it is keyed
    m_group = m_course.nextGroup(GroupLength);
    m_position = 0;
    emit groupChanged();
}
