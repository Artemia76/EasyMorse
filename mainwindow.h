#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "generateur.h"
#include "morse.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
private slots:
    void on_actionQuitter_triggered();
    void volumeChanged(int value);
    void deviceChanged(int index);
    void handleStateChanged(QAudio::State newState);

    void on_m_pbSend_clicked();

private:
    Ui::MainWindow *ui;
    void initializeAudio(const QAudioDeviceInfo &deviceInfo);

    QScopedPointer<Generateur> m_generator;
    QScopedPointer<QAudioOutput> m_audioOutput;
    morse m_Morse;
    bool m_playing_phrase;
};
#endif // MAINWINDOW_H
