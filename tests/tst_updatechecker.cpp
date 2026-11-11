// Tests for core/net/updatechecker - part of EasyMorse, GPLv3

#include <QTest>

#include <QDateTime>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "net/updatechecker.h"
#include "settings/settings.h"

class TestUpdateChecker : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    int m_files = 0;

    // A GitHub reply saved in a file: no network in the tests
    QUrl release(const QByteArray& pJson)
    {
        const QString path = m_dir.filePath(QString("release%1.json").arg(++m_files));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        file.write(pJson);
        return QUrl::fromLocalFile(path);
    }
    static QByteArray reply(const char* pTag)
    {
        return QByteArray("{\"tag_name\": \"") + pTag
             + "\", \"html_url\": \"https://github.com/Artemia76/EasyMorse/releases/tag/" + pTag + "\"}";
    }
    // Waits for the end of a check
    static bool done(UpdateChecker& pChecker)
    {
        return QTest::qWaitFor([&] { return pChecker.state() != UpdateChecker::Checking; }, 5000);
    }

private slots:
    void parsesReleases()
    {
        QString version;
        QUrl url;
        QVERIFY(UpdateChecker::parseRelease(reply("v2.3.0"), version, url));
        QCOMPARE(version, QString("2.3.0"));
        QCOMPARE(url, QUrl("https://github.com/Artemia76/EasyMorse/releases/tag/v2.3.0"));
        QVERIFY(UpdateChecker::parseRelease(reply("2.10.1"), version, url));
        QCOMPARE(version, QString("2.10.1"));
        // Not releases to offer
        QVERIFY(!UpdateChecker::parseRelease("not json", version, url));
        const QByteArray notFound = "{\"message\": \"Not Found\"}";
        QVERIFY(!UpdateChecker::parseRelease(notFound, version, url));
        QVERIFY(!UpdateChecker::parseRelease(reply("v2.3.0-beta"), version, url));
        const QByteArray prerelease = "{\"tag_name\": \"v3.0.0\", \"prerelease\": true}";
        QVERIFY(!UpdateChecker::parseRelease(prerelease, version, url));
        // A page elsewhere than GitHub is not opened: the releases page is
        const QByteArray elsewhere = "{\"tag_name\": \"v2.3.0\", \"html_url\": \"https://example.com/x\"}";
        QVERIFY(UpdateChecker::parseRelease(elsewhere, version, url));
        QCOMPARE(url, QUrl("https://github.com/Artemia76/EasyMorse/releases/latest"));
    }

    void comparesVersions()
    {
        QVERIFY(UpdateChecker::isNewer("2.3.0", "2.2.0"));
        QVERIFY(UpdateChecker::isNewer("2.10.0", "2.9.3"));
        QVERIFY(UpdateChecker::isNewer("3.0", "2.9.9"));
        QVERIFY(!UpdateChecker::isNewer("2.2.0", "2.2.0"));
        QVERIFY(!UpdateChecker::isNewer("2.1.1", "2.2.0"));
        QVERIFY(!UpdateChecker::isNewer("", "2.2.0"));
    }

    void tellsAboutANewerVersion()
    {
        Settings settings(m_dir.filePath("newer.ini"));
        UpdateChecker checker(&settings, "2.2.0");
        checker.setUrl(release(reply("v2.3.0")));
        QSignalSpy found(&checker, &UpdateChecker::newVersion);
        checker.check();
        QVERIFY(done(checker));
        QCOMPARE(checker.state(), UpdateChecker::Available);
        QCOMPARE(checker.latestVersion(), QString("2.3.0"));
        QCOMPARE(found.count(), 1);
        QVERIFY(checker.isNotifying());
        QVERIFY(settings.lastUpdateCheck().isValid());
        // Hidden until the next start
        checker.dismiss();
        QVERIFY(!checker.isNotifying());
    }

    void skipsAVersion()
    {
        Settings settings(m_dir.filePath("skip.ini"));
        {
            UpdateChecker checker(&settings, "2.2.0");
            checker.setUrl(release(reply("v2.3.0")));
            checker.check();
            QVERIFY(done(checker));
            checker.skip();
            QVERIFY(!checker.isNotifying());
        }
        // Not told again about 2.3.0, but about the next one
        UpdateChecker checker(&settings, "2.2.0");
        checker.setUrl(release(reply("v2.3.0")));
        checker.check();
        QVERIFY(done(checker));
        QVERIFY(!checker.isNotifying());
        checker.setUrl(release(reply("v2.4.0")));
        checker.check();
        QVERIFY(done(checker));
        QVERIFY(checker.isNotifying());
    }

    void upToDateOrFailed()
    {
        Settings settings(m_dir.filePath("same.ini"));
        UpdateChecker checker(&settings, "2.2.0");
        checker.setUrl(release(reply("v2.2.0")));
        checker.check();
        QVERIFY(done(checker));
        QCOMPARE(checker.state(), UpdateChecker::UpToDate);
        QVERIFY(!checker.isNotifying());

        Settings other(m_dir.filePath("failed.ini"));
        UpdateChecker failing(&other, "2.2.0");
        failing.setUrl(QUrl::fromLocalFile(m_dir.filePath("missing.json")));
        failing.check();
        QVERIFY(done(failing));
        QCOMPARE(failing.state(), UpdateChecker::Failed);
        // Tried again later: the failure is not a check
        QVERIFY(!other.lastUpdateCheck().isValid());
    }

    void checksOnceADayWhenAllowed()
    {
        Settings settings(m_dir.filePath("due.ini"));
        UpdateChecker checker(&settings, "2.2.0");
        checker.setUrl(release(reply("v2.3.0")));
        settings.setCheckUpdates(false);
        checker.checkIfDue();
        QCOMPARE(checker.state(), UpdateChecker::Idle);

        settings.setCheckUpdates(true);
        settings.setLastUpdateCheck(QDateTime::currentDateTimeUtc().addSecs(-3600));
        checker.checkIfDue();
        QCOMPARE(checker.state(), UpdateChecker::Idle);

        settings.setLastUpdateCheck(QDateTime::currentDateTimeUtc().addDays(-2));
        checker.checkIfDue();
        QVERIFY(done(checker));
        QCOMPARE(checker.state(), UpdateChecker::Available);
    }
};

QTEST_GUILESS_MAIN(TestUpdateChecker)
#include "tst_updatechecker.moc"
