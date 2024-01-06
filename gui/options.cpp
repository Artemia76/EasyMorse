/****************************************************************************
**
** Copyright (C) 2020 Gianni Peschiutta
** Contact: https://bitbucket.org/Artemia/easymorse/src/master/
**
** FFS2Play is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** FFS2Play is distributed in the hope that it will be useful,
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
* options.cpp is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#include "options.h"
#include "ui_options.h"

Options::Options(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Options)
{
    ui->setupUi(this);
    m_settings.beginGroup("audio");
    //Populate Controls
    m_settings.endGroup();
    m_settings.beginGroup("serial");
    ui->m_enableSerial->setEnabled(m_settings.value("PortEnable",false).toBool());
    m_serialPortName = m_settings.value("PortName",
#ifdef Q_OS_LINUX
    "tty0"
#else
#ifdef Q_OS_WIN
    "COM1"
#else
#ifdef Q_OS_MAC
    "tty0"
#endif
#endif
#endif
    ).toString();
    m_settings.endGroup();
    // Iterat available serial ports
    for (auto& serialPort : QSerialPortInfo::availablePorts())
    {
        ui->m_serialBox->addItem(serialPort.portName());
    }
    ui->m_serialBox->setCurrentIndex(ui->m_serialBox->findText(m_serialPortName));
}

void Options::on_buttonBox_accepted()
{
    m_serialPortName = ui->m_serialBox->currentText();
    m_settings.beginGroup("serial");
    m_settings.setValue("PortName", m_serialPortName);
    m_settings.setValue("PortEnable", ui->m_enableSerial->isChecked());
    m_settings.endGroup();
}

