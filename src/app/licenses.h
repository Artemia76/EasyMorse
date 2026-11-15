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

/****************************************************************************
* licenses.h is part of EasyMorse project
*
* Licenses of EasyMorse and of the software it ships, read from the notices
* embedded at build time (packaging/licenses/licenses.json, made by
* packaging/licenses/generate.py). Shown by the About card of the options.
* **************************************************************************/

#ifndef LICENSES_H
#define LICENSES_H

#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Licenses : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList components READ components CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

public:
    explicit Licenses(QObject* parent = nullptr);

    ///
    /// \brief components lists the software shipped with EasyMorse, EasyMorse first:
    /// group, name, version, license (SPDX expression), copyright, url, usage, platforms
    ///
    QVariantList components() const { return m_components; }

    ///
    /// \brief qtVersion is the Qt the application runs on
    ///
    QString     qtVersion() const;

    ///
    /// \brief licenseText gives the full text of every license named in an SPDX
    /// expression, each one under its identifier
    ///
    Q_INVOKABLE QString licenseText(const QString& pExpression) const;

    ///
    /// \brief licenseIds splits an SPDX expression into its license identifiers
    ///
    static QStringList licenseIds(const QString& pExpression);

private:
    QVariantList    m_components;
    QJsonObject     m_texts;
};

#endif // LICENSES_H
