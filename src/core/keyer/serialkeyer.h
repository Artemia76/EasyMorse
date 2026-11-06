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
* serialkeyer.h is part of EasyMorse project
*
* Straight key wired to a serial port control line (DSR or CTS), the other
* side of the key being on DTR which is kept active.
* **************************************************************************/

#ifndef SERIALKEYER_H
#define SERIALKEYER_H

#include <QSerialPort>
#include <QStringList>
#include <QTimer>

#include "keyer/keyerinput.h"

class SerialKeyer : public KeyerInput
{
    Q_OBJECT
    Q_PROPERTY(QString portName READ portName NOTIFY openChanged)
    Q_PROPERTY(bool open READ isOpen NOTIFY openChanged)

public:
    enum class Line
    {
        Dsr,
        Cts
    };
    Q_ENUM(Line)

    explicit SerialKeyer(QObject* parent = nullptr);
    ~SerialKeyer() override;

    static QStringList availablePorts();

    ///
    /// \brief open the port and start watching the key line
    /// \return false if the port does not exist or cannot be opened (see errorOccurred)
    ///
    bool    open(const QString& pPortName);
    void    close();
    bool    isOpen() const;
    QString portName() const;

    void    setLine(Line pLine) { m_line = pLine; }
    Line    line() const { return m_line; }

signals:
    void    openChanged();
    void    errorOccurred(const QString& pMessage);

private slots:
    void    poll();

private:
    QSerialPort m_port;
    QTimer      m_pollTimer;
    Line        m_line = Line::Dsr;
};

#endif // SERIALKEYER_H
