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
#include <QTableWidgetItem>
#include <QTimer>
#include <QtSerialPort>
#include "morse/sequencer.h"
#include "tools/clogger.h"
#include "morse/keydecoder.h"

#include "audio/AudioEngine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
            MainWindow(AudioEngine* pAudio, QWidget *parent = nullptr);
            ~MainWindow();
    bool    PlayMorseMessage(const QString& pMessage);
    void    StopMorseMessage();

protected:
    void    keyPressEvent(QKeyEvent* event);
    void    keyReleaseEvent(QKeyEvent* event);
    void    keyerOn();
    void    keyerOff();

private slots:
    void    onLog(const QString& pMessage,QColor pColor, CL_DEBUG_LEVEL pLevel);
    void    onLogContextMenuRequested(const QPoint &pos);
    void    clearLog();
    void    saveLog();
    void    on_m_pbSend_clicked();
    void    onFrequencyChanged(int value);
    void    onVolumeChanged(int value);
    void    onWordSpeedChanged(int value);
    void    onNoiseCorChanged(int value);
    void    onNoiseFilterChanged(int value);
    void    onFarnsWorthChanged(bool value);
    void    onCharSpeedChanged(int value);
    void    on_m_TableGlossaire_itemDoubleClicked(QTableWidgetItem *item);
    void    on_timer();
    void    onCharDecoded(const QString& pText, const QString& pPattern);
    void    updateDecoderStatus();
    void    onPlaybackFinished();
    void    onCharPlayed(int pIndex);
    void    onActionAFOptionsTriggered();
    void    on_rb_filterNone_toggled(bool checked);
    void    on_rb_filterNormal_toggled(bool checked);
    void    on_rb_filterVerbose_toggled(bool checked);
    void    on_cb_filter_currentTextChanged(const QString &arg1);

signals:
    void    Keyer(bool value);

private:
    Ui::MainWindow*                 ui;

    //AUDIO
    AudioEngine*                    m_audio;

    //Morse
    MorseSequencer                  m_sequencer;
    bool                            m_farnsWorth;
    QString                         m_playedText;
    bool                            m_playing_phrase;
    bool                            m_playing_key;
    CLogger*                        m_log;
    KeyDecoder                      m_decoder;
    QLabel*                         m_decoderStatus;
    QString                         m_decoderPattern;
    QSettings                       m_settings;
    int                             m_maxLine;
    int                             m_frequency;
    int                             m_volume;
    qreal                           m_noiseCorrelation;
    QSerialPort                     m_serial;
    int                             m_noiseFilter;
    int                             m_wordSpeed;
    int                             m_charSpeed;
    bool                            m_serialEnable;
    QString                         m_serialPortName;
    QString                         m_audioDeviceName;
    int                             m_audioPeriodMs;
    QTimer                          m_timer;
    bool                            m_DSR;
    void                            initialize();
    void                            initializeSerial(const QString& pSerialPortName);
    void                            loadDeviceSettings();
    void                            loadDecoderSettings(bool pRestoreEstimates);
    void                            startAudio();
    void                            closeEvent(QCloseEvent* pEvent);
};
#endif // MAINWINDOW_H
