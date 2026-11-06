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
* glossarymodel.cpp is part of EasyMorse project
* **************************************************************************/

#include "glossarymodel.h"

#include "morse/morsecodec.h"

GlossaryModel::GlossaryModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int GlossaryModel::rowCount(const QModelIndex& pParent) const
{
    return pParent.isValid() ? 0 : static_cast<int>(MorseCodec::table().size());
}

QVariant GlossaryModel::data(const QModelIndex& pIndex, int pRole) const
{
    if (!pIndex.isValid() || pIndex.row() >= MorseCodec::table().size())
        return {};
    const auto& entry = MorseCodec::table().at(pIndex.row());
    switch (pRole)
    {
    case Qt::DisplayRole:
    case CharacterRole:
        return QString(entry.first);
    case PatternRole:
        return entry.second;
    }
    return {};
}

QHash<int, QByteArray> GlossaryModel::roleNames() const
{
    return {{CharacterRole, "character"}, {PatternRole, "pattern"}};
}
