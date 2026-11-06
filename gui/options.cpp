/****************************************************************************
**
** Copyright (C) 2020 Gianni Peschiutta
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
* options.cpp is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#include "options.h"
#include "ui_options.h"

Options::Options(const QStringList& pAudioDevices, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Options)
{
    ui->setupUi(this);
    m_settings.beginGroup("audio");
    m_audioDeviceName = m_settings.value("DeviceName").toString();
    // Empty name means the system default output
    ui->m_deviceBox->addItem(tr("System default"), QString());
    for (const QString& device : pAudioDevices)
    {
        ui->m_deviceBox->addItem(device, device);
    }
    const int deviceIndex = ui->m_deviceBox->findData(m_audioDeviceName);
    ui->m_deviceBox->setCurrentIndex(deviceIndex >= 0 ? deviceIndex : 0);
    ui->m_periodBox->setValue(m_settings.value("PeriodMs",DefaultAudioPeriodMs).toInt());
    m_settings.endGroup();
    m_settings.beginGroup("serial");
    ui->m_enableSerial->setChecked(m_settings.value("PortEnable",false).toBool());
    m_serialPortName = m_settings.value("PortName").toString();
    m_settings.endGroup();
    m_settings.beginGroup("decoder");
    ui->m_autoAdapt->setChecked(m_settings.value("AutoAdapt",true).toBool());
    m_settings.endGroup();
    // Iterat available serial ports
    for (auto& serialPort : QSerialPortInfo::availablePorts())
    {
        ui->m_serialBox->addItem(serialPort.portName());
    }
    ui->m_serialBox->setCurrentIndex(ui->m_serialBox->findText(m_serialPortName));
}

Options::~Options()
{
    delete ui;
}

void Options::on_buttonBox_accepted()
{
    m_audioDeviceName = ui->m_deviceBox->currentData().toString();
    m_settings.beginGroup("audio");
    m_settings.setValue("DeviceName", m_audioDeviceName);
    m_settings.setValue("PeriodMs", ui->m_periodBox->value());
    m_settings.endGroup();
    m_serialPortName = ui->m_serialBox->currentText();
    m_settings.beginGroup("serial");
    m_settings.setValue("PortName", m_serialPortName);
    m_settings.setValue("PortEnable", ui->m_enableSerial->isChecked());
    m_settings.endGroup();
    m_settings.beginGroup("decoder");
    m_settings.setValue("AutoAdapt", ui->m_autoAdapt->isChecked());
    m_settings.endGroup();
}

