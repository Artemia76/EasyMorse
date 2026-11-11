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
* updatechecker.h is part of EasyMorse project
*
* Tells the user when a newer version is published: asks GitHub for the
* latest release (nothing about the user is sent), at most once a day while
* the setting allows it, or when asked. A version can be skipped.
* **************************************************************************/

#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>

class QNetworkReply;
class Settings;

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QUrl releaseUrl READ releaseUrl NOTIFY changed)
    // Newer version not skipped nor dismissed: the banner is shown
    Q_PROPERTY(bool notifying READ isNotifying NOTIFY changed)
    // No published package for this platform (iOS)
    Q_PROPERTY(bool supported READ isSupported CONSTANT)

public:
    enum State { Idle, Checking, UpToDate, Available, Failed };
    Q_ENUM(State)

    static constexpr const char* DefaultUrl = "https://api.github.com/repos/Artemia76/EasyMorse/releases/latest";
    static constexpr int CheckIntervalHours = 24;

    UpdateChecker(Settings* pSettings, const QString& pCurrentVersion, QObject* parent = nullptr);

    State   state() const { return m_state; }
    QString currentVersion() const { return m_currentVersion; }
    QString latestVersion() const { return m_latestVersion; }
    QUrl    releaseUrl() const { return m_releaseUrl; }
    bool    isNotifying() const;
    bool    isSupported() const;

    ///
    /// \brief setUrl replaces the GitHub address (tests: a file:// URL)
    ///
    void    setUrl(const QUrl& pUrl) { m_url = pUrl; }
    ///
    /// \brief startAutomatic looks for a new version soon, then every few
    /// hours, each time only when the setting allows it and a day has passed
    ///
    void    startAutomatic(int pFirstDelayMs = 5000);

    Q_INVOKABLE void check();
    Q_INVOKABLE void checkIfDue();
    ///
    /// \brief skip: not told about this latest version again
    ///
    Q_INVOKABLE void skip();
    ///
    /// \brief dismiss hides the banner until the next start
    ///
    Q_INVOKABLE void dismiss();

    ///
    /// \brief version ("2.3.0", from a "v2.3.0" tag) and page of a GitHub release
    /// \return false when the reply is not a release
    ///
    static bool parseRelease(const QByteArray& pJson, QString& pVersion, QUrl& pUrl);
    static bool isNewer(const QString& pCandidate, const QString& pCurrent);

signals:
    void    changed();
    ///
    /// \brief newVersion is emitted once per check that finds a newer version
    ///
    void    newVersion(const QString& pVersion);

private:
    void    finished(QNetworkReply* pReply);
    void    setState(State pState);

    Settings*               m_settings;
    QString                 m_currentVersion;
    QUrl                    m_url { QString::fromLatin1(DefaultUrl) };
    QNetworkAccessManager   m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer                  m_timer;
    State                   m_state = Idle;
    QString                 m_latestVersion;
    QUrl                    m_releaseUrl;
    bool                    m_dismissed = false;
};

#endif // UPDATECHECKER_H
