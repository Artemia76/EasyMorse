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
* glossarymodel.h is part of EasyMorse project
*
* Morse code table as a list model for the glossary page
* **************************************************************************/

#ifndef GLOSSARYMODEL_H
#define GLOSSARYMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

class GlossaryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Role
    {
        CharacterRole = Qt::UserRole + 1,
        PatternRole
    };

    explicit GlossaryModel(QObject* parent = nullptr);

    int         rowCount(const QModelIndex& pParent = QModelIndex()) const override;
    QVariant    data(const QModelIndex& pIndex, int pRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};

#endif // GLOSSARYMODEL_H
