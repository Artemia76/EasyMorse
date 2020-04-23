/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
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
* mainwindow.h is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QTimer>

#include "generator.h"
#include "morse.h"
#include "clogger.h"
#include "analyze.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
            MainWindow(QWidget *parent = nullptr);
            ~MainWindow();
    bool    PlayMorseMessage(const QString& pMessage);
    void    StopMorseMessage();

protected:
    void    keyPressEvent(QKeyEvent* event);
    void    keyReleaseEvent(QKeyEvent* event);
    void    applicationStateChanged(Qt::ApplicationState state);

private slots:
    void    onDeviceChanged(int index);
    void    onOutputAudioStateChanged(QAudio::State newState);
    void    onLog(const QString& pMessage,QColor pColor, CL_DEBUG_LEVEL pLevel);
    void    onLogContextMenuRequested(const QPoint &pos);
    void    clearLog();
    void    saveLog();
    void    on_m_pbSend_clicked();
    void    onFrequencyChanged(int value);
    void    onVolumeChanged(int value);
    void    onSpeedChanged(int value);
    void    onNoiseCorChanged(int value);
    void    onNoiseFilterChanged(int value);
    void    onFarnsWorthChanged(bool value);
    void    on_m_TableGlossaire_itemDoubleClicked(QTableWidgetItem *item);

private:
    Ui::MainWindow*                 ui;
    QScopedPointer<CGenerator>      m_generator;
    QScopedPointer<QAudioOutput>    m_audioOutput;
    CMorse                          m_morse;
    bool                            m_playing_phrase;
    bool                            m_playing_key;
    CLogger*                        m_log;
    CAnalyze*                       m_analyzer;
    QSettings                       m_settings;
    int                             m_maxLine;
    int                             m_frequency;
    qreal                           m_volume;
    qreal                           m_noiseCorrelation;
    int                             m_noiseFilter;
    int                             m_speed;
    void                            initializeAudio(const QAudioDeviceInfo &deviceInfo);
    void                            closeEvent(QCloseEvent* pEvent);
};
#endif // MAINWINDOW_H
