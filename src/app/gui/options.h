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
* options.h is part of EasyMorse project
*
* Options dialog: sound card, serial key, decoder and Morse speeds
* **************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

class Trainer;

QT_BEGIN_NAMESPACE
namespace Ui { class Options; }
QT_END_NAMESPACE

class Options : public QDialog
{
    Q_OBJECT

public:
    Options(Trainer* pTrainer, QWidget *parent = nullptr);
    ~Options();

private slots:
    void on_buttonBox_accepted();
    void updateSpeedLabels();

private:
    Ui::Options*                 ui;
    Trainer*                     m_trainer;
};

#endif // OPTIONS_H
