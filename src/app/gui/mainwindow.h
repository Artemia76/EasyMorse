/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
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
* mainwindow.h is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSettings>
#include <QTableWidgetItem>

#include "tools/clogger.h"

class Settings;
class Trainer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
            MainWindow(Trainer* pTrainer, QWidget *parent = nullptr);
            ~MainWindow();
    bool    PlayMorseMessage(const QString& pMessage);
    void    StopMorseMessage();

protected:
    void    keyPressEvent(QKeyEvent* event);
    void    keyReleaseEvent(QKeyEvent* event);

private slots:
    void    onLog(const QString& pMessage,QColor pColor, CL_DEBUG_LEVEL pLevel);
    void    onLogContextMenuRequested(const QPoint &pos);
    void    clearLog();
    void    saveLog();
    void    on_m_pbSend_clicked();
    void    on_m_TableGlossaire_itemDoubleClicked(QTableWidgetItem *item);
    void    onCharDecoded(const QString& pText, const QString& pPattern);
    void    updateDecoderStatus();
    void    onPlayingChanged();
    void    onCharPlayed(int pIndex);
    void    onActionAFOptionsTriggered();
    void    on_rb_filterNone_toggled(bool checked);
    void    on_rb_filterNormal_toggled(bool checked);
    void    on_rb_filterVerbose_toggled(bool checked);
    void    on_cb_filter_currentTextChanged(const QString &arg1);

private:
    void    bindSettings();
    void    updateLabels();
    void    closeEvent(QCloseEvent* pEvent);

    Ui::MainWindow*                 ui;
    Trainer*                        m_trainer;
    Settings*                       m_morseSettings;
    CLogger*                        m_log;
    QLabel*                         m_decoderStatus;
    QString                         m_playedText;
    // Window and log settings, specific to this user interface
    QSettings                       m_settings;
    int                             m_maxLine;
};
#endif // MAINWINDOW_H
