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

#include <QMediaDevices>
#include "options.h"
#include "ui_options.h"

Options::Options(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Options)
{
    ui->setupUi(this);
    m_settings.beginGroup("synth");
    ui->m_volumeSlider->setValue(qRound(m_settings.value("SoundLevel", 1.0).toReal() * 100));
    ui->m_frequencySlider->setValue(m_settings.value("SoundFreq",900).toInt());
    ui->m_NoiseCorr->setValue(qRound(m_settings.value("NoiseCorrelation",0.0).toReal()*100));
    ui->m_NoiseFilterSlider->setValue(m_settings.value("NoiseFilter",0).toInt());
    m_settings.endGroup();
    m_settings.beginGroup("morse");
    ui->m_CharSpeed->setValue(m_settings.value("CharSpeed", 18).toInt());
    ui->m_UseFarnsWorth->setChecked(m_settings.value("FarnsWorth",true).toBool());
    ui->m_WordSpeed->setValue(m_settings.value("WordSpeed", 12).toInt());
    m_settings.endGroup();
    m_settings.beginGroup("audio");
    //Populate Controls
    // Iterate available sound devices
    for (QAudioDevice &deviceInfo: QMediaDevices::audioOutputs())
    {
            ui->m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    }
    m_audioDeviceName = m_settings.value("DeviceName","").toString();
    ui->m_deviceBox->setCurrentIndex(ui->m_deviceBox->findText(m_audioDeviceName));
    m_settings.endGroup();
    m_settings.beginGroup("serial");
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

void Options::on_m_volumeSlider_valueChanged(int value)
{
    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(value));
}

void Options::on_m_frequencySlider_valueChanged(int value)
{
    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(value));
}

void Options::on_m_NoiseCorr_valueChanged(int value)
{
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(value));
}

void Options::on_m_NoiseFilterSlider_valueChanged(int value)
{
    ui->m_LowPassFilter->setText(QString(tr("Low Pass Filter = %1 %")).arg(value));
}

void Options::on_m_CharSpeed_valueChanged(int value)
{
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(value));
}

void Options::on_m_WordSpeed_valueChanged(int value)
{
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(value));
}
