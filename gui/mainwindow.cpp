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
* mainwindow.cpp is part of EasyMorse project
*
* This class define the main window of the application
* **************************************************************************/

#include <gui/mainwindow.h>
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>
#include <QSerialPortInfo>
#include <QStatusBar>

#include <tools/version.h>
#include <gui/options.h>

MainWindow::MainWindow(AudioEngine* pAudio, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_audio(pAudio)
    , m_sequencer(pAudio)
    , m_DSR(false)
{
    ui->setupUi(this);
    connect(ui->actionafOptions, &QAction::triggered, this, &MainWindow::onActionAFOptionsTriggered);
    // Init window position and size
    m_settings.beginGroup("MainWindow");
    restoreGeometry(m_settings.value("Geometry").toByteArray());
    //restoreState(m_settings.value("State").toByteArray());
    m_settings.endGroup();
    // Setting Log
    m_log = CLogger::instance();
    connect(m_log, &CLogger::fireLog, this, &MainWindow::onLog);
    m_settings.beginGroup("Log");
    m_log->setDebugLevel(m_settings.value("Level",LEVEL_NONE).value<CL_DEBUG_LEVEL>());
    m_log->log("Main Window started...",Qt::magenta,LEVEL_NORMAL);
    switch (m_log->DebugLevel)
    {
        case LEVEL_NONE:
        {
            ui->rb_filterNone->setChecked(true);
            break;
        }
        case LEVEL_NORMAL:
        {
            ui->rb_filterNormal->setChecked(true);
            break;
        }
        case LEVEL_VERBOSE:
        {
            ui->rb_filterVerbose->setChecked(true);
            break;
        }
    }
    int id=ui->cb_filter->findText(m_log->getFilter(),Qt::MatchExactly);
    if (id >=0)
    {
        ui->cb_filter->setCurrentIndex(id);
    }
    else
    {
        ui->cb_filter->addItem(m_log->getFilter());
        ui->cb_filter->setCurrentIndex(ui->cb_filter->findText(m_log->getFilter(),Qt::MatchExactly));
    }
    m_maxLine = m_settings.value("MaxLine",200).toInt();
    m_settings.endGroup();

    m_settings.beginGroup("Synth");

    //Set Sound Frequency
    connect(ui->m_frequencySlider, &QSlider::valueChanged, this, &MainWindow::onFrequencyChanged);
    m_frequency = m_settings.value("SoundFreq",900).toInt();
    m_audio->setFrequency(m_frequency);

    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(m_frequency));

    //Set Sound Volume
    connect(ui->m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    m_volume = m_settings.value("SoundLevel", 100).toInt();
    m_audio->setGain(m_volume);

    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(m_volume));

    //Set NoiseCorrelation
    connect(ui->m_NoiseCorr, &QSlider::valueChanged, this, &MainWindow::onNoiseCorChanged);
    m_noiseCorrelation = m_settings.value("NoiseCorrelation",0.0).toReal();
    m_audio->setNoiseRatio(m_noiseCorrelation);
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(m_noiseCorrelation*100));
    m_audio->noiseOn();
    //Set noise Filter
    connect(ui->m_NoiseFilterSlider, &QSlider::valueChanged, this, &MainWindow::onNoiseFilterChanged);
    m_noiseFilter = m_settings.value("NoiseFilter",0).toInt();
    m_audio->setNoiseCutoff(m_noiseFilter);
    ui->m_LowPassFilter->setText(QString(tr("Low Pass Filter = %1 %")).arg(m_noiseFilter));

    m_settings.endGroup();
    m_settings.beginGroup("Morse");

    //Set FarnsWorth Option
    connect(ui->m_UseFarnsWorth, &QCheckBox::checkStateChanged, this, &MainWindow::onFarnsWorthChanged);
    m_farnsWorth = m_settings.value("FarnsWorth",true).toBool();
    connect(&m_sequencer, &MorseSequencer::finished, this, &MainWindow::onPlaybackFinished);
    connect(&m_sequencer, &MorseSequencer::charPlayed, this, &MainWindow::onCharPlayed);

    //Set Word Speed
    connect(ui->m_WordSpeed, &QSlider::valueChanged, this, &MainWindow::onWordSpeedChanged);
    m_wordSpeed = m_settings.value("WordSpeed", 12).toInt();
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(m_wordSpeed));

    //Set Char Speed
    connect(ui->m_CharSpeed, &QSlider::valueChanged, this, &MainWindow::onCharSpeedChanged);
    m_charSpeed = m_settings.value("CharSpeed", 18).toInt();
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(m_charSpeed));
    m_settings.endGroup();

    loadDeviceSettings();
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::on_timer);
    m_timer.setSingleShot(false);
    m_timer.start(10);

    // LogZone Initialize
    ui->m_LogZone->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->m_LogZone, &QPlainTextEdit::customContextMenuRequested, this, &MainWindow::onLogContextMenuRequested);
    QPalette p = ui->m_LogZone->palette();
    p.setColor(QPalette::Text,Qt::darkBlue);
    ui->m_LogZone->setPalette(p);
    ui->m_LogZone->setMaximumBlockCount(m_maxLine);
    m_log->log(tr("Welcome to EasyMorse"),Qt::blue);

    if (QFontDatabase::addApplicationFont(":/fonts/morse.ttf")==-1)
        ui->m_LogZone->appendPlainText(tr("Failed to load font"));

    //Format glossary with morse font
    QFont Morse("morse",12, QFont::Normal);
    for (int i = 0; i < ui->m_TableGlossaire->rowCount(); i++)
    {
        ui->m_TableGlossaire->item(i,0)->setFont(Morse);
        ui->m_TableGlossaire->item(i,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }

    // Initialize
    initialize();

    if (m_serialEnable)
        initializeSerial(m_serialPortName);

    m_playing_phrase=false;
    m_playing_key=false;
    // Streaming decoder of the local key
    m_decoderStatus = new QLabel(this);
    statusBar()->addPermanentWidget(m_decoderStatus);
    connect(this, &MainWindow::Keyer, &m_decoder, &KeyDecoder::keyEdgeNow);
    connect(&m_decoder, &KeyDecoder::charDecoded, this, &MainWindow::onCharDecoded);
    connect(&m_decoder, &KeyDecoder::wordGap, this, [this] {
        ui->m_MorseZone->moveCursor(QTextCursor::End);
        ui->m_MorseZone->insertPlainText(" ");
    });
    connect(&m_decoder, &KeyDecoder::symbolProgress, this, [this](const QString& pPattern) {
        m_decoderPattern = pPattern;
        updateDecoderStatus();
    });
    connect(&m_decoder, &KeyDecoder::speedChanged, this, &MainWindow::updateDecoderStatus);
    loadDecoderSettings(true);

    setWindowTitle(QString(VER_PRODUCTNAME_STR) + " " + QString(VER_PRODUCTVERSION_STR));

    startAudio();
}

MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief MainWindow::closeEvent
///
void MainWindow::closeEvent(QCloseEvent*)
{
    if (m_serial.isOpen()) m_serial.close();
    m_audio->stop();
    m_settings.beginGroup("MainWindow");
    m_settings.setValue("Geometry",saveGeometry());
    m_settings.setValue("State",saveState());
    m_settings.endGroup();
    m_settings.beginGroup("Log");
    m_settings.setValue("Level", m_log->DebugLevel);
    m_settings.setValue("MaxLine",m_maxLine);
    m_settings.endGroup();
    m_settings.beginGroup("Synth");
    m_settings.setValue("SoundFreq",m_frequency);
    m_settings.setValue("SoundLevel", m_volume);
    m_settings.setValue("NoiseCorrelation",m_noiseCorrelation);
    m_settings.setValue("NoiseFilter",m_noiseFilter);
    m_settings.endGroup();
    m_settings.beginGroup("Morse");
    m_settings.setValue("WordSpeed", m_wordSpeed);
    m_settings.setValue("FarnsWorth",m_farnsWorth);
    m_settings.setValue("CharSpeed", m_charSpeed);
    m_settings.endGroup();
    m_settings.beginGroup("decoder");
    m_settings.setValue("DotMs", m_decoder.dotMs());
    m_settings.setValue("DashMs", m_decoder.dashMs());
    m_settings.setValue("GapMs", m_decoder.gapMs());
    m_settings.endGroup();
}

///
/// \brief MainWindow::initialize
///
void MainWindow::initialize()
{
    ui->m_volumeSlider->setValue(m_volume);
    ui->m_frequencySlider->setValue(m_frequency);
    ui->m_WordSpeed->setValue(m_wordSpeed);
    ui->m_NoiseCorr->setValue(qRound(m_noiseCorrelation*100));
    ui->m_NoiseFilterSlider->setValue(m_noiseFilter);
    ui->m_UseFarnsWorth->setChecked(m_farnsWorth);
    ui->m_CharSpeed->setValue(m_charSpeed);
}

///
/// \brief MainWindow::initializeSerial
/// \param pSerialPortName
///
void MainWindow::initializeSerial(const QString &pSerialPortName)
{
    QSerialPortInfo portInfo;
    for (auto& serialPort : QSerialPortInfo::availablePorts())
    {
        if (serialPort.portName()==pSerialPortName)
        {
            portInfo = serialPort;
        }
    }
    if (!portInfo.isNull())
    {
        if (m_serial.isOpen()) m_serial.close();
        m_serial.setPort(portInfo);
        m_serial.setFlowControl(QSerialPort::NoFlowControl);
        if (!m_serial.open(QIODevice::ReadWrite))
        {
            m_log->log(tr("Failed to open Serial port..."),Qt::red);
        }
        m_serial.setDataTerminalReady(true);
        m_serial.setRequestToSend(false);
    }
    else
    {
        m_log->log(tr("Serial port %1 not found").arg(pSerialPortName),Qt::darkMagenta);
    }
}

///
/// \brief MainWindow::loadDeviceSettings reads the serial key and sound card settings
///
void MainWindow::loadDeviceSettings()
{
    m_settings.beginGroup("serial");
    m_serialEnable = m_settings.value("PortEnable",false).toBool();
    m_serialPortName = m_settings.value("PortName").toString();
    m_settings.endGroup();
    m_settings.beginGroup("audio");
    m_audioDeviceName = m_settings.value("DeviceName").toString();
    m_audioPeriodMs = m_settings.value("PeriodMs",Options::DefaultAudioPeriodMs).toInt();
    m_settings.endGroup();
}

///
/// \brief MainWindow::startAudio (re)opens the sound card
///
void MainWindow::startAudio()
{
    if (!m_audio->start(m_audioDeviceName, m_audioPeriodMs))
        m_log->log(tr("No sound output, check the audio options"),Qt::red);
}

///
/// \brief MainWindow::onVolumeChanged
/// \param value
///
void MainWindow::onVolumeChanged(int value)
{
    m_volume = value;
    m_audio->setGain(m_volume);
    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(value));
}

///
/// \brief MainWindow::onFrequencyChanged
/// \param value
///
void MainWindow::onFrequencyChanged(int value)
{
    m_frequency=value;
    m_audio->setFrequency(value);
    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(value));
}

///
/// \brief MainWindow::onWordSpeedChanged
/// \param value
///
void MainWindow::onWordSpeedChanged(int value)
{
    m_wordSpeed = value;
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(m_wordSpeed));
}

///
/// \brief MainWindow::onCharSpeedChanged
/// \param value
///
void MainWindow::onCharSpeedChanged(int value)
{
    m_charSpeed = value;
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(m_charSpeed));
    // A decoder locked on the set speed follows the slider
    if (!m_decoder.autoAdapt())
        m_decoder.setSpeed(m_charSpeed);
}

///
/// \brief MainWindow::onFarnsWorthChanged
/// \param value
///
void MainWindow::onFarnsWorthChanged(bool value)
{
    m_farnsWorth = value;
}

///
/// \brief MainWindow::onNoiseCorChanged
/// \param index
///
void MainWindow::onNoiseCorChanged(int index)
{
    m_noiseCorrelation = static_cast<qreal>(index / 100.0);
    m_audio->setNoiseRatio(m_noiseCorrelation);
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(index));
}

///
/// \brief MainWindow::onNoiseFilterChanged
/// \param index
///
void MainWindow::onNoiseFilterChanged(int index)
{
    m_noiseFilter = index;
    m_audio->setNoiseCutoff(m_noiseFilter);
    ui->m_LowPassFilter->setText(QString(tr("Low Pass Filter = %1 Hz")).arg(index));
}

///
/// \brief MainWindow::keyPressEvent
/// \param event
///
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Alt))
    {
#ifdef QT_DEBUG
        m_log->log("keyPressEvent occured",Qt::darkMagenta,LEVEL_VERBOSE);
#endif
        keyerOn();
        grabKeyboard();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

///
/// \brief MainWindow::keyReleaseEvent
/// \param event
///
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Alt)
    {
#ifdef QT_DEBUG
        m_log->log("keyReleaseEvent occured",Qt::darkMagenta,LEVEL_VERBOSE);
#endif
        keyerOff();
        releaseKeyboard();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

///
/// \brief MainWindow::on_m_pbSend_clicked
///
void MainWindow::on_m_pbSend_clicked()
{
    if (!m_playing_phrase)
    {
        PlayMorseMessage(ui->m_leMessage->text());
    }
    else
    {
        StopMorseMessage();
    }
}

///
/// \brief MainWindow::PlayMorseMessage
/// \param pMessage
/// \return
///
bool MainWindow::PlayMorseMessage(const QString& pMessage)
{
    if (!m_playing_phrase)
    {
        if (pMessage.isEmpty()) return false;
        if (!m_sequencer.play(pMessage, MorseTiming::fromSpeed(m_wordSpeed, m_charSpeed, m_farnsWorth)))
        {
            m_log->log(tr("Nothing to play, or no sound output"),Qt::darkMagenta);
            return false;
        }
        m_playedText = pMessage;
        m_playing_phrase=true;
        ui->m_frequencySlider->setDisabled(true);
        //ui->m_volumeSlider->setDisabled(true);
        ui->m_WordSpeed->setDisabled(true);
        ui->m_CharSpeed->setDisabled(true);
        ui->m_NoiseCorr->setDisabled(true);
        ui->m_NoiseFilterSlider->setDisabled(true);
        ui->m_pbSend->setText(tr("Stop"));
        return true;
    }
    return false;
}

///
/// \brief MainWindow::StopMorseMessage
///
void MainWindow::StopMorseMessage()
{
    // finished() is emitted, which restores the controls
    m_sequencer.stop();
}

///
/// \brief MainWindow::onPlaybackFinished
///
void MainWindow::onPlaybackFinished()
{
    if (m_playing_phrase)
    {
        m_playing_phrase=false;
        ui->m_frequencySlider->setDisabled(false);
        //ui->m_volumeSlider->setDisabled(false);
        ui->m_WordSpeed->setDisabled(false);
        ui->m_CharSpeed->setDisabled(false);
        ui->m_NoiseCorr->setDisabled(false);
        ui->m_NoiseFilterSlider->setDisabled(false);
        ui->m_pbSend->setText(tr("Play"));
    }
}

///
/// \brief MainWindow::onLog
/// \param Texte
/// \param pColor
/// \param pLevel
///
/// Redirect Log Text flow to Plain Text Edit on main Window
///
void MainWindow::onLog (const QString& pMessage, QColor pColor, CL_DEBUG_LEVEL)
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(pColor));
    ui->m_LogZone->mergeCurrentCharFormat(fmt);
    ui->m_LogZone->appendPlainText(pMessage);
}

///
/// \brief MainWindow::on_pteLogger_customContextMenuRequested
/// \param pos
///
void MainWindow::onLogContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = ui->m_LogZone->createStandardContextMenu();
    // Handle global position
    QPoint globalPos = ui->m_LogZone->mapToGlobal(pos);
    menu->addAction(tr("Clear"), this, &MainWindow::clearLog);
    menu->addAction(tr("Save"), this, &MainWindow::saveLog);
    menu->exec(globalPos);
    delete menu;
}

///
/// \brief MainWindow::clearLog
///
void MainWindow::clearLog()
{
    ui->m_LogZone->clear();
}

///
/// \brief MainWindow::saveLog
///
void MainWindow::saveLog()
{
    QFileDialog FileDlg(this);
    QString File;
    FileDlg.setAcceptMode(QFileDialog::AcceptSave);
    FileDlg.setDefaultSuffix("htm");
    FileDlg.setNameFilter(tr("Web File (*.htm *.html)"));
    if (FileDlg.exec())
    {
        File = FileDlg.selectedFiles().constFirst();
        if (!File.isEmpty())
        {
            QTextDocument* Doc = ui->m_LogZone->document();
            QFile document (File);
            if(!document.open(QFile::WriteOnly | QFile::Text))
            {
                m_log->log(tr("An Error occur while opening ") + document.fileName(),Qt::darkMagenta);
                return;
            }
            QTextStream writer (&document);
            writer << Doc->toHtml();
        }
    }
}

///
/// \brief MainWindow::on_m_TableGlossaire_itemDoubleClicked
/// \param item
///
void MainWindow::on_m_TableGlossaire_itemDoubleClicked(QTableWidgetItem *item)
{
    PlayMorseMessage(item->text());
}

///
/// \brief MainWindow::on_timer
///
void MainWindow::on_timer()
{
    if (m_serial.isOpen())
    {
        QSerialPort::PinoutSignals state = m_serial.pinoutSignals();
        //bool CTS = (state & QSerialPort::ClearToSendSignal) != 0;
        bool DSR = (state & QSerialPort::DataSetReadySignal) != 0;
        if (m_playing_phrase) return;
        if (DSR && (!m_DSR) && (!m_playing_key))
        {
            keyerOn();
        }
        if ((!DSR) && (m_DSR) && (m_playing_key))
        {
            keyerOff();
        }
        m_DSR=DSR;
    }
}

///
/// \brief MainWindow::keyerOn
///
void MainWindow::keyerOn()
{
    if (m_playing_phrase) return;
    if (!m_playing_key)
    {
        m_audio->keyDown(KeySource::Local);
        m_playing_key=true;
        emit Keyer(true);
    }
}

///
/// \brief MainWindow::keyerOff
///
void MainWindow::keyerOff()
{
    if (m_playing_phrase) return;
    if (m_playing_key)
    {
        m_audio->keyUp(KeySource::Local);
        m_playing_key=false;
        emit Keyer(false);
    }
}

///
/// \brief MainWindow::onActionAFOptionsTriggered
///
void MainWindow::onActionAFOptionsTriggered()
{
    Options DlgOptions(m_audio->outputDevices(), this);
    if (DlgOptions.exec()!=QDialog::Accepted)
        return;
    const QString oldDevice = m_audioDeviceName;
    const int oldPeriod = m_audioPeriodMs;
    loadDeviceSettings();
    // Serial key
    if (m_serial.isOpen()) m_serial.close();
    m_DSR = false;
    if (m_serialEnable)
        initializeSerial(m_serialPortName);
    loadDecoderSettings(false);
    // Sound card
    StopMorseMessage();
    if ((m_audioDeviceName != oldDevice) || (m_audioPeriodMs != oldPeriod) || !m_audio->isRunning())
        startAudio();
}

///
/// \brief MainWindow::onMorseMessage
/// \param pMessage
///
void MainWindow::onCharDecoded(const QString& pText, const QString& pPattern)
{
    // Unknown patterns are shown as received to help the learner
    ui->m_MorseZone->moveCursor(QTextCursor::End);
    ui->m_MorseZone->insertPlainText(pText.isEmpty() ? "[" + pPattern + "]" : pText);
    m_decoderPattern.clear();
    updateDecoderStatus();
}

///
/// \brief MainWindow::updateDecoderStatus shows the estimated speed and the character being keyed
///
void MainWindow::updateDecoderStatus()
{
    m_decoderStatus->setText(tr("Decoder: %1 WPM%2  %3")
                             .arg(qRound(m_decoder.speed()))
                             .arg(m_decoder.autoAdapt() ? QString() : tr(" (locked)"))
                             .arg(m_decoderPattern));
}

///
/// \brief MainWindow::loadDecoderSettings
/// \param pRestoreEstimates start from the speed learnt in the previous session
///
void MainWindow::loadDecoderSettings(bool pRestoreEstimates)
{
    m_settings.beginGroup("decoder");
    const bool autoAdapt = m_settings.value("AutoAdapt",true).toBool();
    const double dot = m_settings.value("DotMs",0.0).toDouble();
    const double dash = m_settings.value("DashMs",0.0).toDouble();
    const double gap = m_settings.value("GapMs",0.0).toDouble();
    m_settings.endGroup();
    m_decoder.setAutoAdapt(autoAdapt);
    if (!autoAdapt)
        m_decoder.setSpeed(m_charSpeed);
    else if (pRestoreEstimates)
    {
        m_decoder.setSpeed(m_charSpeed);
        m_decoder.setEstimates(dot, dash, gap);
    }
    updateDecoderStatus();
}

///
/// \brief MainWindow::onCharPlayed highlights the character being played
/// \param pIndex
///
void MainWindow::onCharPlayed(int pIndex)
{
    if (ui->m_leMessage->text() == m_playedText)
        ui->m_leMessage->setSelection(pIndex, 1);
}


///
/// \brief MainWindow::on_rb_filterNone_toggled
/// \param checked
///
void MainWindow::on_rb_filterNone_toggled(bool checked)
{
    if (checked) m_log->setDebugLevel(LEVEL_NONE);
}

///
/// \brief MainWindow::on_rb_filterNormal_toggled
/// \param checked
///
void MainWindow::on_rb_filterNormal_toggled(bool checked)
{
    if (checked) m_log->setDebugLevel(LEVEL_NORMAL);
}

///
/// \brief MainWindow::on_rb_filterVerbose_toggled
/// \param checked
///
void MainWindow::on_rb_filterVerbose_toggled(bool checked)
{
    if (checked) m_log->setDebugLevel(LEVEL_VERBOSE);
}

///
/// \brief MainWindow::on_cb_filter_currentTextChanged
/// \param arg1
///

void MainWindow::on_cb_filter_currentTextChanged(const QString &arg1)
{
    m_log->setFilter(arg1);
}

