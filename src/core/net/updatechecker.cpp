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
* updatechecker.cpp is part of EasyMorse project
* **************************************************************************/

#include "updatechecker.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>

#include "settings/settings.h"

UpdateChecker::UpdateChecker(Settings* pSettings, const QString& pCurrentVersion, QObject* parent)
    : QObject(parent)
    , m_settings(pSettings)
    , m_currentVersion(pCurrentVersion)
{
    // A skipped version changes what is shown
    connect(m_settings, &Settings::updatesChanged, this, &UpdateChecker::changed);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        // After the first check, every few hours for a program left open for days
        m_timer.setInterval(3 * 3600 * 1000);
        checkIfDue();
    });
}

bool UpdateChecker::isSupported() const
{
#ifdef Q_OS_IOS
    return false;
#else
    return true;
#endif
}

bool UpdateChecker::isNotifying() const
{
    return m_state == Available && !m_dismissed && m_latestVersion != m_settings->skippedVersion();
}

void UpdateChecker::startAutomatic(int pFirstDelayMs)
{
    if (!isSupported())
        return;
    m_timer.setInterval(pFirstDelayMs);
    m_timer.start();
}

void UpdateChecker::checkIfDue()
{
    if (!m_settings->checkUpdates())
        return;
    const QDateTime last = m_settings->lastUpdateCheck();
    if (last.isValid() && last.secsTo(QDateTime::currentDateTimeUtc()) < CheckIntervalHours * 3600)
        return;
    check();
}

void UpdateChecker::check()
{
    if (m_reply || !isSupported())
        return;
    QNetworkRequest request(m_url);
    // A plain client name: nothing about the user
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("EasyMorse/%1").arg(m_currentVersion));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(15000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply.data()] { finished(reply); });
    setState(Checking);
}

void UpdateChecker::finished(QNetworkReply* pReply)
{
    pReply->deleteLater();
    m_reply.clear();
    QString version;
    QUrl url;
    if (pReply->error() != QNetworkReply::NoError || !parseRelease(pReply->readAll(), version, url))
    {
        // Tried again at the next start or in a few hours
        setState(Failed);
        return;
    }
    m_settings->setLastUpdateCheck(QDateTime::currentDateTimeUtc());
    m_latestVersion = version;
    m_releaseUrl = url;
    if (isNewer(version, m_currentVersion))
    {
        setState(Available);
        emit newVersion(version);
    }
    else
    {
        setState(UpToDate);
    }
}

void UpdateChecker::setState(State pState)
{
    m_state = pState;
    emit changed();
}

void UpdateChecker::skip()
{
    if (!m_latestVersion.isEmpty())
        m_settings->setSkippedVersion(m_latestVersion);
}

void UpdateChecker::dismiss()
{
    if (m_dismissed)
        return;
    m_dismissed = true;
    emit changed();
}

bool UpdateChecker::parseRelease(const QByteArray& pJson, QString& pVersion, QUrl& pUrl)
{
    const QJsonObject release = QJsonDocument::fromJson(pJson).object();
    if (release.value("draft").toBool() || release.value("prerelease").toBool())
        return false;
    QString tag = release.value("tag_name").toString().trimmed();
    if (tag.startsWith(u'v') || tag.startsWith(u'V'))
        tag.remove(0, 1);
    qsizetype suffix = 0;
    const QVersionNumber number = QVersionNumber::fromString(tag, &suffix);
    // "2.3.0" only: no "2.3.0-beta"
    if (number.isNull() || suffix != tag.size())
        return false;
    pVersion = number.toString();
    pUrl = QUrl(release.value("html_url").toString());
    // Opened in the browser: only a page of GitHub
    if (!pUrl.isValid() || pUrl.scheme() != u"https" || pUrl.host() != u"github.com")
        pUrl = QUrl("https://github.com/Artemia76/EasyMorse/releases/latest");
    return true;
}

bool UpdateChecker::isNewer(const QString& pCandidate, const QString& pCurrent)
{
    const QVersionNumber candidate = QVersionNumber::fromString(pCandidate);
    const QVersionNumber current = QVersionNumber::fromString(pCurrent);
    return !candidate.isNull() && QVersionNumber::compare(candidate, current) > 0;
}
