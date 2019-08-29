#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFontDatabase>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    ui->m_deviceBox->addItem(defaultDeviceInfo.deviceName(), qVariantFromValue(defaultDeviceInfo));
    if (QFontDatabase::addApplicationFont(":/fonts/morse.ttf")==-1)
        ui->m_LogZone->appendPlainText("Failed to load font");
    QFont Morse("morse",12, QFont::Normal);
    for (int i = 0; i < ui->m_TableGlossaire->rowCount(); i++)
    {
        ui->m_TableGlossaire->item(i,0)->setFont(Morse);
    }
    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if (deviceInfo != defaultDeviceInfo)
            ui->m_deviceBox->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
    }
    connect(ui->m_deviceBox, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::deviceChanged);
    connect(ui->m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    initializeAudio(QAudioDeviceInfo::defaultOutputDevice());
    connect(m_audioOutput.data(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    m_playing_phrase=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionQuitter_triggered()
{

}

void MainWindow::initializeAudio(const QAudioDeviceInfo &deviceInfo)
{
    m_generator.reset(new Generateur());
    QAudioFormat format = m_generator->getFormat();

    if (!deviceInfo.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = deviceInfo.nearestFormat(format);
    }

    m_generator->generateData(1000000, true);
    m_audioOutput.reset(new QAudioOutput(deviceInfo,format));
    m_generator->start();

    qreal initialVolume = QAudio::convertVolume(m_audioOutput->volume(),
                                                QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    ui->m_volumeSlider->setValue(qRound(initialVolume * 100));
    m_audioOutput->start(m_generator.data());
    m_audioOutput->suspend();
}

void MainWindow::volumeChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
        QAudio::LogarithmicVolumeScale,
        QAudio::LinearVolumeScale);
    m_audioOutput->setVolume(linearVolume);
}

void MainWindow::deviceChanged(int index)
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
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    //ui->plainTextEdit->appendPlainText("keyReleaseEvent occured");
    if (event->key() == Qt::Key_Alt)
    {
        if (m_playing_phrase) return;
        m_audioOutput->suspend();
    }
}

void MainWindow::handleStateChanged(QAudio::State newState)
{
  //ui->m_LogZone->appendPlainText("Audio State changed");
  switch (newState) {
      case QAudio::IdleState:
          // Finished playing (no more data)
          m_audioOutput->stop();
          ui->m_LogZone->appendPlainText("End of playing sound");
          m_Morse.data()->stop();
          m_playing_phrase=false;
          m_audioOutput->start(m_generator.data());
          m_audioOutput->suspend();
          break;

      case QAudio::StoppedState:
          // Stopped for other reasons
          if (m_audioOutput->error() != QAudio::NoError)
          {
              // Error handling
          }
          break;

      default:
          // ... other cases as appropriate
          break;
  }
}

void MainWindow::on_m_pbSend_clicked()
{
    if (!m_playing_phrase)
    {
        QString Message = ui->m_leMessage->text();
        if (Message.isEmpty()) return;
        m_Morse.code(Message);
        m_audioOutput->start(m_Morse.data());
        m_playing_phrase=true;
    }
}
