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

#include <QFontDatabase>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>
#include <QStatusBar>

#include "version.h"
#include "settings/settings.h"
#include "trainer.h"
#include <gui/options.h>

MainWindow::MainWindow(Trainer* pTrainer, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_trainer(pTrainer)
    , m_morseSettings(pTrainer->settings())
{
    ui->setupUi(this);
    connect(ui->actionafOptions, &QAction::triggered, this, &MainWindow::onActionAFOptionsTriggered);
    // Init window position and size
    m_settings.beginGroup("MainWindow");
    restoreGeometry(m_settings.value("Geometry").toByteArray());
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

    bindSettings();

    // Playback
    connect(m_trainer, &Trainer::playingChanged, this, &MainWindow::onPlayingChanged);
    connect(m_trainer, &Trainer::charPlayed, this, &MainWindow::onCharPlayed);

    // Streaming decoder of the local key
    m_decoderStatus = new QLabel(this);
    statusBar()->addPermanentWidget(m_decoderStatus);
    connect(m_trainer, &Trainer::charDecoded, this, &MainWindow::onCharDecoded);
    connect(m_trainer, &Trainer::wordGap, this, [this] {
        ui->m_MorseZone->moveCursor(QTextCursor::End);
        ui->m_MorseZone->insertPlainText(" ");
    });
    connect(m_trainer, &Trainer::decoderPatternChanged, this, &MainWindow::updateDecoderStatus);
    connect(m_trainer, &Trainer::decoderSpeedChanged, this, &MainWindow::updateDecoderStatus);
    connect(m_morseSettings, &Settings::decoderAutoAdaptChanged, this, &MainWindow::updateDecoderStatus);
    updateDecoderStatus();

    setWindowTitle(QString(VER_PRODUCTNAME_STR) + " " + QString(VER_PRODUCTVERSION_STR));
}

MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief MainWindow::bindSettings keeps the sound and speed controls in sync with the settings
///
void MainWindow::bindSettings()
{
    ui->m_frequencySlider->setValue(m_morseSettings->toneFrequency());
    ui->m_volumeSlider->setValue(m_morseSettings->volume());
    ui->m_NoiseCorr->setValue(qRound(m_morseSettings->noiseLevel()*100));
    ui->m_NoiseFilterSlider->setValue(m_morseSettings->noiseFilter());
    ui->m_WordSpeed->setValue(m_morseSettings->wordSpeed());
    ui->m_CharSpeed->setValue(m_morseSettings->charSpeed());
    ui->m_UseFarnsWorth->setChecked(m_morseSettings->farnsworth());
    updateLabels();

    connect(ui->m_frequencySlider, &QSlider::valueChanged, m_morseSettings, &Settings::setToneFrequency);
    connect(ui->m_volumeSlider, &QSlider::valueChanged, m_morseSettings, &Settings::setVolume);
    connect(ui->m_NoiseCorr, &QSlider::valueChanged, this, [this](int pValue) {
        m_morseSettings->setNoiseLevel(pValue / 100.0);
    });
    connect(ui->m_NoiseFilterSlider, &QSlider::valueChanged, m_morseSettings, &Settings::setNoiseFilter);
    connect(ui->m_WordSpeed, &QSlider::valueChanged, m_morseSettings, &Settings::setWordSpeed);
    connect(ui->m_CharSpeed, &QSlider::valueChanged, m_morseSettings, &Settings::setCharSpeed);
    connect(ui->m_UseFarnsWorth, &QCheckBox::toggled, m_morseSettings, &Settings::setFarnsworth);

    for (auto signal : {&Settings::toneFrequencyChanged, &Settings::volumeChanged, &Settings::noiseLevelChanged,
                        &Settings::noiseFilterChanged, &Settings::wordSpeedChanged, &Settings::charSpeedChanged})
        connect(m_morseSettings, signal, this, &MainWindow::updateLabels);
}

///
/// \brief MainWindow::updateLabels
///
void MainWindow::updateLabels()
{
    ui->m_labelFreq->setText(QString(tr("Sound Freq=%1 Hz")).arg(m_morseSettings->toneFrequency()));
    ui->m_labelSoundLevel->setText(QString(tr("Sound Level = %1 %")).arg(m_morseSettings->volume()));
    ui->m_labelNoiseMixing->setText(QString(tr("Noise Mixing = %1 %")).arg(qRound(m_morseSettings->noiseLevel()*100)));
    ui->m_LowPassFilter->setText(QString(tr("Low Pass Filter = %1 Hz")).arg(m_morseSettings->noiseFilter()));
    ui->m_labelWordSpeed->setText(QString(tr("Word Speed = %1 WPM")).arg(m_morseSettings->wordSpeed()));
    ui->m_labelCharSpeed->setText(QString(tr("Char Speed = %1 WPM")).arg(m_morseSettings->charSpeed()));
}

///
/// \brief MainWindow::closeEvent
///
void MainWindow::closeEvent(QCloseEvent*)
{
    m_trainer->stop();
    m_settings.beginGroup("MainWindow");
    m_settings.setValue("Geometry",saveGeometry());
    m_settings.setValue("State",saveState());
    m_settings.endGroup();
    m_settings.beginGroup("Log");
    m_settings.setValue("Level", m_log->DebugLevel);
    m_settings.setValue("MaxLine",m_maxLine);
    m_settings.endGroup();
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
        m_trainer->manualKeyer()->press();
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
        m_trainer->manualKeyer()->release();
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
    if (!m_trainer->isPlaying())
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
    if (m_trainer->isPlaying() || pMessage.isEmpty())
        return false;
    m_playedText = pMessage;
    return m_trainer->play(pMessage);
}

///
/// \brief MainWindow::StopMorseMessage
///
void MainWindow::StopMorseMessage()
{
    m_trainer->stop();
}

///
/// \brief MainWindow::onPlayingChanged locks the controls that would change the text being played
///
void MainWindow::onPlayingChanged()
{
    const bool playing = m_trainer->isPlaying();
    ui->m_frequencySlider->setDisabled(playing);
    ui->m_WordSpeed->setDisabled(playing);
    ui->m_CharSpeed->setDisabled(playing);
    ui->m_NoiseCorr->setDisabled(playing);
    ui->m_NoiseFilterSlider->setDisabled(playing);
    ui->m_pbSend->setText(playing ? tr("Stop") : tr("Play"));
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

///
/// \brief MainWindow::on_m_TableGlossaire_itemDoubleClicked
/// \param item
///
void MainWindow::on_m_TableGlossaire_itemDoubleClicked(QTableWidgetItem *item)
{
    PlayMorseMessage(item->text());
}

///
/// \brief MainWindow::onActionAFOptionsTriggered
///
void MainWindow::onActionAFOptionsTriggered()
{
    // Options write the settings; the trainer applies them
    Options DlgOptions(m_trainer, this);
    DlgOptions.exec();
}

///
/// \brief MainWindow::onCharDecoded
/// \param pText
/// \param pPattern
///
void MainWindow::onCharDecoded(const QString& pText, const QString& pPattern)
{
    // Unknown patterns are shown as received to help the learner
    ui->m_MorseZone->moveCursor(QTextCursor::End);
    ui->m_MorseZone->insertPlainText(pText.isEmpty() ? "[" + pPattern + "]" : pText);
}

///
/// \brief MainWindow::updateDecoderStatus shows the estimated speed and the character being keyed
///
void MainWindow::updateDecoderStatus()
{
    m_decoderStatus->setText(tr("Decoder: %1 WPM%2  %3")
                             .arg(qRound(m_trainer->decoderSpeed()))
                             .arg(m_morseSettings->decoderAutoAdapt() ? QString() : tr(" (locked)"))
                             .arg(m_trainer->decoderPattern()));
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
