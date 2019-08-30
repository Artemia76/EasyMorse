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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Init window position and size
    m_settings.beginGroup("MainWindow");
    restoreGeometry(m_settings.value("Geometry").toByteArray());
    restoreState(m_settings.value("State").toByteArray());
    m_settings.endGroup();

    m_settings.beginGroup("log");
    m_maxLine = m_settings.value("MaxLine",200).toInt();
    m_settings.endGroup();

    m_settings.beginGroup("synth");
    m_frequency = m_settings.value("SoundFreq",900).toInt();
    m_volume = m_settings.value("SoundLevel", 1.0).toReal();
    m_speed = m_settings.value("MorseSpeed", 100).toInt();
    m_morse.setSpeed(m_speed);
    m_settings.endGroup();

    m_log = CLogger::instance();
    connect(m_log, SIGNAL(fireLog(QString,QColor,CL_DEBUG_LEVEL)),this,SLOT(onLog(QString,QColor,CL_DEBUG_LEVEL)),Qt::QueuedConnection);

    // LogZone Initialize
    ui->m_LogZone->setContextMenuPolicy(Qt::CustomContextMenu);
    QPalette p = ui->m_LogZone->palette();
    p.setColor(QPalette::Text,Qt::darkBlue);
    ui->m_LogZone->setPalette(p);
    ui->m_LogZone->setMaximumBlockCount(m_maxLine);
    m_log->log(tr("Welcome to EasyMorse"),Qt::blue);

    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    ui->m_deviceBox->addItem(defaultDeviceInfo.deviceName(), qVariantFromValue(defaultDeviceInfo));
    if (QFontDatabase::addApplicationFont(":/fonts/morse.ttf")==-1)
        ui->m_LogZone->appendPlainText("Failed to load font");
    //Format glossary with morse font
    QFont Morse("morse",12, QFont::Normal);
    for (int i = 0; i < ui->m_TableGlossaire->rowCount(); i++)
    {
        ui->m_TableGlossaire->item(i,0)->setFont(Morse);
    }

    // Iterate available sound devices
    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if (deviceInfo != defaultDeviceInfo)
            ui->m_deviceBox->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
    }
    connect(ui->m_deviceBox, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onDeviceChanged);

    //Slots connexion
    initializeAudio(QAudioDeviceInfo::defaultOutputDevice());
    m_playing_phrase=false;
    m_playing_key=false;

    connect(m_audioOutput.data(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(onOutputAudioStateChanged(QAudio::State)),Qt::QueuedConnection);
    connect(ui->m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(ui->m_frequencySlider, &QSlider::valueChanged, this, &MainWindow::onFrenquencyChanged);
    connect(ui->m_MorseSpeed, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
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
    m_settings.beginGroup("MainWindow");
    m_settings.setValue("Geometry",saveGeometry());
    m_settings.setValue("State",saveState());
    m_settings.endGroup();
    m_settings.beginGroup("Log");
    m_settings.setValue("MaxLine",m_maxLine);
    m_settings.endGroup();
    m_settings.beginGroup("synth");
    m_settings.setValue("SoundFreq",m_frequency);
    m_settings.setValue("SoundLevel", m_volume);
    m_settings.setValue("MorseSpeed", m_speed);
    m_settings.endGroup();
}


void MainWindow::initializeAudio(const QAudioDeviceInfo &deviceInfo)
{
    m_generator.reset(new CGenerator());
    QAudioFormat format = m_generator->getFormat();

    if (!deviceInfo.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = deviceInfo.nearestFormat(format);
    }
    m_generator->setFrequency(m_frequency);
    m_morse.setFrequency(m_frequency);
    m_generator->generateData(1000000, true);
    m_audioOutput.reset(new QAudioOutput(deviceInfo,format));
    m_generator->start();
    m_audioOutput->setVolume(m_volume);
    qreal slidervolume = QAudio::convertVolume(m_volume,
        QAudio::LinearVolumeScale,
        QAudio::LogarithmicVolumeScale);
    ui->m_volumeSlider->setValue(qRound(slidervolume * 100));
    ui->m_frequencySlider->setValue(m_frequency);
    ui->m_MorseSpeed->setValue(m_speed);
    m_audioOutput->start(m_generator.data());
    m_audioOutput->suspend();
}

void MainWindow::onVolumeChanged(int value)
{
    m_volume = QAudio::convertVolume(value / qreal(100),
        QAudio::LogarithmicVolumeScale,
        QAudio::LinearVolumeScale);
    m_audioOutput->setVolume(m_volume);
    m_audioOutput->start(m_generator.data());
    if (!m_playing_key)
    {
        m_audioOutput->suspend();
    }
}

void MainWindow::onFrenquencyChanged(int value)
{
    m_frequency=value;
    m_generator->stop();
    m_generator->setFrequency(m_frequency);
    m_generator->generateData(1000000,true);
    m_morse.setFrequency(m_frequency);
    m_generator->start();
    m_audioOutput->start(m_generator.data());
    if (!m_playing_key)
    {
        m_audioOutput->suspend();
    }
}

void MainWindow::onSpeedChanged(int value)
{
    m_speed = value;
    m_morse.setSpeed(m_speed);
}

void MainWindow::onDeviceChanged(int index)
{
    m_generator->stop();
    m_audioOutput->stop();
    m_audioOutput->disconnect(this);
    initializeAudio(ui->m_deviceBox->itemData(index).value<QAudioDeviceInfo>());
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    //ui->plainTextEdit->appendPlainText("keyPressEvent occured");
    if ((event->key() == Qt::Key_Alt))
    {
        if (m_playing_phrase) return;
        m_audioOutput->resume();
        m_playing_key=true;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    //ui->plainTextEdit->appendPlainText("keyReleaseEvent occured");
    if (event->key() == Qt::Key_Alt)
    {
        m_playing_key=false;
        if (m_playing_phrase) return;
        m_audioOutput->suspend();
    }
}

void MainWindow::onOutputAudioStateChanged(QAudio::State pNewState)
{
    //ui->m_LogZone->appendPlainText("Audio State changed");
    switch (pNewState)
    {
        case QAudio::IdleState:
        {
            if (m_playing_phrase)
                on_m_pbSend_clicked();
            break;
        }
        case QAudio::StoppedState:
        {
            // Stopped for other reasons
            if (m_audioOutput->error() != QAudio::NoError)
            {
                // Error handling
            }
            break;
        }
        default:
        {
            // ... other cases as appropriate
            break;
        }
    }
}

void MainWindow::on_m_pbSend_clicked()
{
    if (!m_playing_phrase)
    {
        PlayMorseMessage(ui->m_leMessage->text());
        ui->m_pbSend->setText(tr("Stop"));
    }
    else
    {
        StopMorseMessage();
        ui->m_pbSend->setText(tr("Play"));
    }
}

void MainWindow::PlayMorseMessage(const QString& pMessage)
{
    if (!m_playing_phrase)
    {
        if (pMessage.isEmpty()) return;
        m_morse.code(pMessage);
        m_audioOutput->start(m_morse.data());
        m_playing_phrase=true;
        ui->m_deviceBox->setDisabled(true);
        ui->m_frequencySlider->setDisabled(true);
        ui->m_volumeSlider->setDisabled(true);
        ui->m_MorseSpeed->setDisabled(true);
    }
}

void MainWindow::StopMorseMessage()
{
    if (m_playing_phrase)
    {
        // Finished playing (no more data)
        m_audioOutput->stop();
        m_morse.data()->stop();
        m_audioOutput->start(m_generator.data());
        m_audioOutput->suspend();
        m_playing_phrase=false;
        ui->m_deviceBox->setDisabled(false);
        ui->m_frequencySlider->setDisabled(false);
        ui->m_volumeSlider->setDisabled(false);
        ui->m_MorseSpeed->setDisabled(false);
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
        File = FileDlg.selectedFiles().first();
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
