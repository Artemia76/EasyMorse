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
* logmodel.h is part of EasyMorse project
*
* Log messages of CLogger as a list model for the journal page
* **************************************************************************/

#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

#include "tools/clogger.h"

class LogModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int maxLines READ maxLines WRITE setMaxLines NOTIFY maxLinesChanged)

public:
    enum Role
    {
        MessageRole = Qt::UserRole + 1,
        ColorRole,
        LevelRole
    };

    explicit LogModel(QObject* parent = nullptr);

    int         rowCount(const QModelIndex& pParent = QModelIndex()) const override;
    QVariant    data(const QModelIndex& pIndex, int pRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int         maxLines() const { return m_maxLines; }
    void        setMaxLines(int pMaxLines);

    // QML singleton backed by the instance created in main(), so that the
    // messages logged before the log page is shown are kept
    inline static LogModel* s_instance = nullptr;
    static LogModel* create(QQmlEngine*, QJSEngine*);

    Q_INVOKABLE void clear();
    ///
    /// \brief saveHtml writes the log as a web page
    /// \return false if the file cannot be written
    ///
    Q_INVOKABLE bool saveHtml(const QUrl& pFile) const;

signals:
    void        maxLinesChanged();

private:
    struct Entry
    {
        QString         message;
        QColor          color;
        CL_DEBUG_LEVEL  level;
    };

    void        append(const QString& pMessage, QColor pColor, CL_DEBUG_LEVEL pLevel);
    void        trim();

    QList<Entry>    m_entries;
    int             m_maxLines = 200;
};

#endif // LOGMODEL_H
