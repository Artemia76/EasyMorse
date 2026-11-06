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
* Options dialog: sound card, serial key, decoder and Morse speeds
* **************************************************************************/

#include "options.h"
#include "ui_options.h"

#include "settings/settings.h"
#include "trainer.h"

Options::Options(Trainer* pTrainer, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Options)
    , m_trainer(pTrainer)
{
    ui->setupUi(this);
    setWindowTitle(tr("Options"));
    const Settings* settings = m_trainer->settings();

    // Sound card: an empty name means the system default output
    ui->m_deviceBox->addItem(tr("System default"), QString());
    for (const QString& device : m_trainer->audioDevices())
    {
        ui->m_deviceBox->addItem(device, device);
    }
    const int deviceIndex = ui->m_deviceBox->findData(settings->audioDevice());
    ui->m_deviceBox->setCurrentIndex(deviceIndex >= 0 ? deviceIndex : 0);
    ui->m_periodBox->setValue(settings->audioPeriodMs());

    // Serial key
#ifdef EASYMORSE_HAS_SERIALPORT
    ui->m_enableSerial->setChecked(settings->serialEnabled());
    ui->m_serialBox->addItems(m_trainer->serialPorts());
    ui->m_serialBox->setCurrentText(settings->serialPort());
#else
    ui->m_enableSerial->setEnabled(false);
    ui->m_serialBox->setEnabled(false);
#endif
    ui->m_autoAdapt->setChecked(settings->decoderAutoAdapt());

    // Morse speeds
    ui->m_WordSpeed->setValue(settings->wordSpeed());
    ui->m_CharSpeed->setValue(settings->charSpeed());
    ui->m_UseFarnsWorth->setChecked(settings->farnsworth());
    connect(ui->m_WordSpeed, &QSlider::valueChanged, this, &Options::updateSpeedLabels);
    connect(ui->m_CharSpeed, &QSlider::valueChanged, this, &Options::updateSpeedLabels);
    updateSpeedLabels();
}

Options::~Options()
{
    delete ui;
}

void Options::updateSpeedLabels()
{
    ui->m_labelWordSpeed->setText(tr("Word Speed = %1 WPM").arg(ui->m_WordSpeed->value()));
    ui->m_labelCharSpeed->setText(tr("Char Speed = %1 WPM").arg(ui->m_CharSpeed->value()));
}

void Options::on_buttonBox_accepted()
{
    Settings* settings = m_trainer->settings();
    settings->setAudioDevice(ui->m_deviceBox->currentData().toString());
    settings->setAudioPeriodMs(ui->m_periodBox->value());
#ifdef EASYMORSE_HAS_SERIALPORT
    settings->setSerialPort(ui->m_serialBox->currentText());
    settings->setSerialEnabled(ui->m_enableSerial->isChecked());
#endif
    settings->setDecoderAutoAdapt(ui->m_autoAdapt->isChecked());
    settings->setWordSpeed(ui->m_WordSpeed->value());
    settings->setCharSpeed(ui->m_CharSpeed->value());
    settings->setFarnsworth(ui->m_UseFarnsWorth->isChecked());
}
