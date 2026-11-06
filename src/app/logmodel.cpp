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
* logmodel.cpp is part of EasyMorse project
* **************************************************************************/

#include "logmodel.h"

#include <QFile>
#include <QJSEngine>
#include <QTextStream>

LogModel* LogModel::create(QQmlEngine*, QJSEngine*)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

LogModel::LogModel(QObject* parent)
    : QAbstractListModel(parent)
{
    connect(CLogger::instance(), &CLogger::fireLog, this, &LogModel::append);
}

int LogModel::rowCount(const QModelIndex& pParent) const
{
    return pParent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant LogModel::data(const QModelIndex& pIndex, int pRole) const
{
    if (!pIndex.isValid() || pIndex.row() >= m_entries.size())
        return {};
    const Entry& entry = m_entries.at(pIndex.row());
    switch (pRole)
    {
    case Qt::DisplayRole:
    case MessageRole:
        return entry.message;
    case ColorRole:
        return entry.color;
    case LevelRole:
        return static_cast<int>(entry.level);
    }
    return {};
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {{MessageRole, "message"}, {ColorRole, "messageColor"}, {LevelRole, "level"}};
}

void LogModel::setMaxLines(int pMaxLines)
{
    pMaxLines = qMax(10, pMaxLines);
    if (pMaxLines == m_maxLines)
        return;
    m_maxLines = pMaxLines;
    trim();
    emit maxLinesChanged();
}

void LogModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

bool LogModel::saveHtml(const QUrl& pFile) const
{
    QFile file(pFile.isLocalFile() ? pFile.toLocalFile() : pFile.toString());
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return false;
    QTextStream out(&file);
    out << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>EasyMorse</title></head><body><pre>\n";
    for (const Entry& entry : m_entries)
        out << "<span style=\"color:" << entry.color.name() << "\">" << entry.message.toHtmlEscaped() << "</span>\n";
    out << "</pre></body></html>\n";
    return true;
}

void LogModel::append(const QString& pMessage, QColor pColor, CL_DEBUG_LEVEL pLevel)
{
    const int row = static_cast<int>(m_entries.size());
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append({pMessage, pColor, pLevel});
    endInsertRows();
    trim();
}

void LogModel::trim()
{
    const int extra = static_cast<int>(m_entries.size()) - m_maxLines;
    if (extra <= 0)
        return;
    beginRemoveRows(QModelIndex(), 0, extra - 1);
    m_entries.remove(0, extra);
    endRemoveRows();
}
