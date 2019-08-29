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
    connect(ui->m_suspendResumeButton, &QPushButton::clicked, this, &MainWindow::toggleSuspendResume);
    connect(ui->m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    initializeAudio(QAudioDeviceInfo::defaultOutputDevice());
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
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if (!deviceInfo.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = deviceInfo.nearestFormat(format);
    }

    const int durationSeconds = 1;
    const int toneSampleRateHz = 880;
    m_generator.reset(new Generateur(format, durationSeconds * 1000000, toneSampleRateHz));
    m_audioOutput.reset(new QAudioOutput(deviceInfo, format));
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

void MainWindow::toggleSuspendResume()
{
    if (m_audioOutput->state() == QAudio::SuspendedState || m_audioOutput->state() == QAudio::StoppedState) {
        m_audioOutput->resume();
        ui->m_suspendResumeButton->setText(tr("Suspend recording"));
    } else if (m_audioOutput->state() == QAudio::ActiveState) {
        m_audioOutput->suspend();
        ui->m_suspendResumeButton->setText(tr("Resume playback"));
    } else if (m_audioOutput->state() == QAudio::IdleState) {
        // no-op
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    //ui->plainTextEdit->appendPlainText("keyPressEvent occured");
    if (event->key() == Qt::Key_Alt) m_audioOutput->resume();
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    //ui->plainTextEdit->appendPlainText("keyReleaseEvent occured");
    if (event->key() == Qt::Key_Alt) m_audioOutput->suspend();
}
