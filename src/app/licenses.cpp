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
** Foundation and appearing in the file LICENSE.GPLv3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

#include "licenses.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QtGlobal>

Licenses::Licenses(QObject* parent)
    : QObject(parent)
{
    QFile file(QStringLiteral(":/licenses/licenses.json"));
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("Licenses: the embedded notices are missing");
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_components = root.value(QStringLiteral("components")).toArray().toVariantList();
    m_texts = root.value(QStringLiteral("texts")).toObject();
}

QString Licenses::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

QStringList Licenses::licenseIds(const QString& pExpression)
{
    static const QRegularExpression token(QStringLiteral("[A-Za-z0-9.+-]+"));
    QStringList ids;
    for (const QRegularExpressionMatch& match : token.globalMatch(pExpression))
    {
        const QString id = match.captured();
        if (id == QLatin1String("AND") || id == QLatin1String("OR") || id == QLatin1String("WITH") || ids.contains(id))
            continue;
        ids.append(id);
    }
    return ids;
}

QString Licenses::licenseText(const QString& pExpression) const
{
    QStringList parts;
    for (const QString& id : licenseIds(pExpression))
    {
        // "or later" uses the text of the version it names (as generate.py does)
        QString key = id;
        if (key.endsWith(QLatin1String("-or-later")))
            key = key.chopped(int(qstrlen("-or-later"))) + QLatin1String("-only");
        const QString text = m_texts.value(key).toString();
        if (!text.isEmpty())
            parts.append(id + QLatin1String("\n\n") + text);
    }
    return parts.join(QLatin1String("\n\n----------\n\n"));
}
