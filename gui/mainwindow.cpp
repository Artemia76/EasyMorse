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

#include <tools/version.h>
#include <gui/options.h>

MainWindow::MainWindow(QWidget *parent, AudioHal* hal, VoiceManager* pVoiceManager)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_hal(hal)
    , m_manager(pVoiceManager)
    , m_dOctaveOsc(0.0)
    , m_morse(nullptr, hal, pVoiceManager)
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
    connect(m_log, SIGNAL(fireLog(QString,QColor,CL_DEBUG_LEVEL)),this,SLOT(onLog(QString,QColor,CL_DEBUG_LEVEL)));
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
    m_morse.setFrequency(m_frequency);

    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(m_frequency));

    //Set Sound Volume
    connect(ui->m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    m_volume = m_settings.value("SoundLevel", 100).toInt();
    //m_generator->setVolume(m_volume);
    m_manager->setGain(m_volume);

    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(m_volume));

    //Set NoiseCorrelation
    connect(ui->m_NoiseCorr, &QSlider::valueChanged, this, &MainWindow::onNoiseCorChanged);
    m_noiseCorrelation = m_settings.value("NoiseCorrelation",0.0).toReal();
    m_manager->setNoiseRatio(m_noiseCorrelation);
    //m_generator->setNoiseCorrelation(m_noiseCorrelation);
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(m_noiseCorrelation*100));
    m_hal->startNoise();
    //Set noise Filter
    m_manager->setFilter(FilterType::LOWPASS);
    connect(ui->m_NoiseFilterSlider, &QSlider::valueChanged, this, &MainWindow::onNoiseFilterChanged);
    m_noiseFilter = m_settings.value("NoiseFilter",0).toInt();
    m_manager->setCutOffFrequency(m_noiseFilter);
    ui->m_LowPassFilter->setText(QString(tr("Low Pass Filter = %1 %")).arg(m_noiseFilter));

    m_settings.endGroup();
    m_settings.beginGroup("Morse");

    //Set FarnsWorth Option
    connect(ui->m_UseFarnsWorth, &QCheckBox::stateChanged, this, &MainWindow::onFarnsWorthChanged);
    m_morse.setFarnsWorth(m_settings.value("FarnsWorth",true).toBool());

    //Set Word Speed
    connect(ui->m_WordSpeed, &QSlider::valueChanged, this, &MainWindow::onWordSpeedChanged);
    m_wordSpeed = m_settings.value("WordSpeed", 12).toInt();
    m_morse.setWordSpeed(m_wordSpeed);
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(m_wordSpeed));

    //Set Char Speed
    connect(ui->m_CharSpeed, &QSlider::valueChanged, this, &MainWindow::onCharSpeedChanged);
    m_charSpeed = m_settings.value("CharSpeed", 18).toInt();
    m_morse.setCharSpeed(m_charSpeed);
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(m_charSpeed));
    m_settings.endGroup();

    m_settings.beginGroup("serial");
    m_serialEnable = m_settings.value("PortEnable",false).toBool();
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
    m_settings.beginGroup("audio");
    m_audioDeviceName = m_settings.value("DeviceName","").toString();
    m_settings.endGroup();
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timer()));
    m_timer.setSingleShot(false);
    m_timer.start(10);

    // LogZone Initialize
    ui->m_LogZone->setContextMenuPolicy(Qt::CustomContextMenu);
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

    // Iterate available sound devices
    //const QAudioDevice &defaultDeviceInfo = m_devices->defaultAudioOutput();
    //ui->m_deviceBox->addItem(defaultDeviceInfo.description(), QVariant::fromValue(defaultDeviceInfo));
    //for (auto &deviceInfo: m_devices->audioOutputs())
    //{
    //    if (deviceInfo != defaultDeviceInfo)
    //        ui->m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    //}

    // Initialize Audio
    initializeAudio();

    // Iterat available serial ports
    //for (auto& serialPort : QSerialPortInfo::availablePorts())
    //{
    //    ui->m_serialBox->addItem(serialPort.portName());
    //}
    //ui->m_serialBox->setCurrentIndex(ui->m_serialBox->findText(m_serialPortName));

    if (m_serialEnable)
        initializeSerial(m_serialPortName);

    m_CTS=false;
    m_playing_phrase=false;
    m_playing_key=false;
    m_analyzer = new CAnalyze(this);
    connect(this,SIGNAL(Keyer(bool)),m_analyzer,SLOT(on_keyer(bool)));
    connect(m_analyzer,SIGNAL(fire_message(QString)), this, SLOT(onMorseMessage(QString)));

    setWindowTitle(QString(VER_PRODUCTNAME_STR) + " " + QString(VER_PRODUCTVERSION_STR));
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
    //if (m_audioOutput != nullptr) m_audioOutput->stop();
    if (m_serial.isOpen()) m_serial.close();
    //m_generator->stop();
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
    //m_settings.setValue("Device",QMediaDevices::defaultAudioOutput().description());
    m_settings.endGroup();
    m_settings.beginGroup("Morse");
    m_settings.setValue("WordSpeed", m_wordSpeed);
    m_settings.setValue("FarnsWorth",m_morse.getFarnsWorth());
    m_settings.setValue("CharSpeed", m_charSpeed);
    m_settings.endGroup();
}

///
/// \brief MainWindow::initializeAudio
///
void MainWindow::initializeAudio()
{
    ui->m_volumeSlider->setValue(m_volume);
    ui->m_frequencySlider->setValue(m_frequency);
    ui->m_WordSpeed->setValue(m_wordSpeed);
    ui->m_NoiseCorr->setValue(qRound(m_noiseCorrelation*100));
    ui->m_NoiseFilterSlider->setValue(m_noiseFilter);
    ui->m_UseFarnsWorth->setChecked(m_morse.getFarnsWorth());
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

}

///
/// \brief MainWindow::onVolumeChanged
/// \param value
///
void MainWindow::onVolumeChanged(int value)
{
    m_volume = value;
    //m_generator->setVolume(value);
    m_manager->setGain(m_volume);
    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(value));
}

///
/// \brief MainWindow::onFrequencyChanged
/// \param value
///
void MainWindow::onFrequencyChanged(int value)
{
    m_frequency=value;
    m_morse.setFrequency(value);
    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(value));
}

///
/// \brief MainWindow::onWordSpeedChanged
/// \param value
///
void MainWindow::onWordSpeedChanged(int value)
{
    m_wordSpeed = value;
    m_morse.setWordSpeed(m_wordSpeed);
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(m_wordSpeed));
}

///
/// \brief MainWindow::onCharSpeedChanged
/// \param value
///
void MainWindow::onCharSpeedChanged(int value)
{
    m_charSpeed = value;
    m_morse.setCharSpeed(m_charSpeed);
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(m_charSpeed));
}

///
/// \brief MainWindow::onFarnsWorthChanged
/// \param value
///
void MainWindow::onFarnsWorthChanged(bool value)
{
    m_morse.setFarnsWorth(value);
}

///
/// \brief MainWindow::onNoiseCorChanged
/// \param index
///
void MainWindow::onNoiseCorChanged(int index)
{
    m_noiseCorrelation = static_cast<qreal>(index / 100.0);
    m_manager->setNoiseRatio(m_noiseCorrelation);
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(index));
}

///
/// \brief MainWindow::onNoiseFilterChanged
/// \param index
///
void MainWindow::onNoiseFilterChanged(int index)
{
    m_noiseFilter = index;
    m_manager->setCutOffFrequency(m_noiseFilter);
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
        m_playing_phrase=true;
        ui->m_frequencySlider->setDisabled(true);
        //ui->m_volumeSlider->setDisabled(true);
        ui->m_WordSpeed->setDisabled(true);
        ui->m_CharSpeed->setDisabled(true);
        ui->m_NoiseCorr->setDisabled(true);
        ui->m_NoiseFilterSlider->setDisabled(true);
        ui->m_pbSend->setText(tr("Stop"));
        m_morse.code(pMessage);
        StopMorseMessage();
        return true;
    }
    return false;
}

///
/// \brief MainWindow::StopMorseMessage
///
void MainWindow::StopMorseMessage()
{
    if (m_playing_phrase)
    {
        // Finished playing (no more data)
        //m_morse->stop();
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
    menu->addAction(tr("Clear"),this,SLOT(clearLog()));
    menu->addAction(tr("Save"),this,SLOT(saveLog()));
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
        m_afrequency = m_frequency;
        m_hal->play(m_afrequency);
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
        m_hal->stop(m_afrequency);
        m_playing_key=false;
        emit Keyer(false);
    }
}

///
/// \brief MainWindow::onActionAFOptionsTriggered
///
void MainWindow::onActionAFOptionsTriggered()
{
    Options* DlgOptions= new Options(this);
    DlgOptions->show();
}

///
/// \brief MainWindow::onMorseMessage
/// \param pMessage
///
void MainWindow::onMorseMessage(QString pMessage)
{
    ui->m_MorseZone->insertPlainText(m_morse.decodeMorse(pMessage) + ' ');
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

