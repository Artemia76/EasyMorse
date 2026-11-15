// Tests for the embedded third-party notices (app/licenses) - part of EasyMorse, GPLv3

#include <QTest>

#include "licenses.h"

class TestLicenses : public QObject
{
    Q_OBJECT

private slots:
    void easyMorseComesFirst()
    {
        Licenses licenses;
        const QVariantList components = licenses.components();
        QVERIFY(components.size() > 10);
        const QVariantMap first = components.first().toMap();
        QCOMPARE(first.value("name").toString(), QString("EasyMorse"));
        QCOMPARE(first.value("license").toString(), QString("GPL-3.0-or-later"));
        QVERIFY(licenses.licenseText("GPL-3.0-or-later").contains("GNU GENERAL PUBLIC LICENSE"));
    }

    void everyLicenseHasItsText()
    {
        Licenses licenses;
        for (const QVariant& component : licenses.components())
        {
            const QVariantMap map = component.toMap();
            const QString expression = map.value("license").toString();
            if (expression.isEmpty())
                continue;  // terms of the publisher (system libraries)
            const QString text = licenses.licenseText(expression);
            for (const QString& id : Licenses::licenseIds(expression))
            {
                if (id == "LicenseRef-Qt-Commercial")
                    continue;  // not the license EasyMorse uses
                QVERIFY2(text.contains(id), qPrintable(map.value("name").toString() + ": no text for " + id));
            }
        }
    }

    void splitsExpressions()
    {
        QCOMPARE(Licenses::licenseIds("(MIT OR Apache-2.0) AND BSD-3-Clause"),
                 QStringList({"MIT", "Apache-2.0", "BSD-3-Clause"}));
        QCOMPARE(Licenses::licenseIds("GPL-2.0-only WITH Linux-syscall-note"),
                 QStringList({"GPL-2.0-only", "Linux-syscall-note"}));
        QCOMPARE(Licenses::licenseIds(""), QStringList());
    }
};

QTEST_GUILESS_MAIN(TestLicenses)
#include "tst_licenses.moc"
