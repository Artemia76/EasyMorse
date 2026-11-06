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
* serialkeyer.cpp is part of EasyMorse project
* **************************************************************************/

#include "serialkeyer.h"

#include <QFileInfo>
#include <QSerialPortInfo>

namespace
{
    // Control lines cannot notify changes: they are polled
    constexpr int PollIntervalMs = 5;
}

SerialKeyer::SerialKeyer(QObject* parent)
    : KeyerInput(parent)
{
    m_pollTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pollTimer, &QTimer::timeout, this, &SerialKeyer::poll);
}

SerialKeyer::~SerialKeyer()
{
    close();
}

QStringList SerialKeyer::availablePorts()
{
    QStringList ports;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts())
        ports.append(info.portName());
    return ports;
}

bool SerialKeyer::open(const QString& pPortName)
{
    close();
    // A device path (/dev/serial/by-id/...) may be given instead of a listed port name
    const bool isPath = pPortName.contains(u'/');
    if (isPath ? !QFileInfo::exists(pPortName) : !availablePorts().contains(pPortName))
    {
        emit errorOccurred(tr("Serial port %1 not found").arg(pPortName));
        return false;
    }
    m_port.setPortName(pPortName);
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port.open(QIODevice::ReadWrite))
    {
        emit errorOccurred(tr("Failed to open serial port %1: %2").arg(pPortName, m_port.errorString()));
        return false;
    }
    // DTR feeds the key, RTS is unused
    m_port.setDataTerminalReady(true);
    m_port.setRequestToSend(false);
    m_pollTimer.start(PollIntervalMs);
    emit openChanged();
    return true;
}

void SerialKeyer::close()
{
    m_pollTimer.stop();
    if (!m_port.isOpen())
        return;
    m_port.close();
    setDown(false);
    emit openChanged();
}

bool SerialKeyer::isOpen() const
{
    return m_port.isOpen();
}

QString SerialKeyer::portName() const
{
    return m_port.isOpen() ? m_port.portName() : QString();
}

void SerialKeyer::poll()
{
    if (m_port.error() == QSerialPort::ResourceError)
    {
        // Adapter unplugged
        emit errorOccurred(tr("Serial port %1 lost: %2").arg(m_port.portName(), m_port.errorString()));
        close();
        return;
    }
    const QSerialPort::PinoutSignals state = m_port.pinoutSignals();
    const QSerialPort::PinoutSignal line = (m_line == Line::Cts) ? QSerialPort::ClearToSendSignal
                                                                 : QSerialPort::DataSetReadySignal;
    setDown(state.testFlag(line));
}
