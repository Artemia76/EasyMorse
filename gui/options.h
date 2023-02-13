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
* options.h is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QSettings>
#include <QSerialPort>
#include <QSerialPortInfo>

QT_BEGIN_NAMESPACE
namespace Ui { class Options; }
QT_END_NAMESPACE

class Options : public QDialog
{
    Q_OBJECT

public:
    Options(QWidget *parent = nullptr);
private slots:
    void on_m_volumeSlider_valueChanged(int value);

    void on_m_frequencySlider_valueChanged(int value);

    void on_m_NoiseCorr_valueChanged(int value);

    void on_m_NoiseFilterSlider_valueChanged(int value);

    void on_m_CharSpeed_valueChanged(int value);

    void on_m_WordSpeed_valueChanged(int value);

private:
    Ui::Options*                 ui;
    QSettings                    m_settings;
    QString                      m_audioDeviceName;
    QString                      m_serialPortName;
};

#endif // OPTIONS_H
