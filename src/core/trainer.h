/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
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
* trainer.h is part of EasyMorse project
*
* Application logic independent of the user interface: plays texts,
* listens to the keyers, decodes them and keeps the audio engine in line
* with the settings.
* **************************************************************************/

#ifndef TRAINER_H
#define TRAINER_H

#include <QObject>
#include <QSet>
#include <QStringList>

#include "audio/AudioEngine.h"
#include "keyer/keyerinput.h"
#include "morse/keydecoder.h"
#include "morse/sequencer.h"

class Settings;
#ifdef EASYMORSE_HAS_SERIALPORT
class SerialKeyer;
#endif

class Trainer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool audioRunning READ isAudioRunning NOTIFY audioRunningChanged)
    Q_PROPERTY(bool keyDown READ isKeyDown NOTIFY keyDownChanged)
    Q_PROPERTY(double decoderSpeed READ decoderSpeed NOTIFY decoderSpeedChanged)
    Q_PROPERTY(QString decoderPattern READ decoderPattern NOTIFY decoderPatternChanged)
    Q_PROPERTY(ManualKeyer* manualKeyer READ manualKeyer CONSTANT)

public:
    explicit Trainer(Settings* pSettings, QObject* parent = nullptr);
    ~Trainer() override;

    ///
    /// \brief start opens the sound card and the serial key chosen in the settings
    ///
    void            start();

    Settings*       settings() const { return m_settings; }
    AudioEngine*    audio() { return &m_audio; }
    KeyDecoder*     decoder() { return &m_decoder; }
    ManualKeyer*    manualKeyer() { return &m_manualKeyer; }
#ifdef EASYMORSE_HAS_SERIALPORT
    SerialKeyer*    serialKeyer() const { return m_serialKeyer; }
#endif

    ///
    /// \brief addKeyer listens to another key (remote station...)
    ///
    void            addKeyer(KeyerInput* pKeyer);
    void            removeKeyer(KeyerInput* pKeyer);

    Q_INVOKABLE QStringList audioDevices();
    Q_INVOKABLE QStringList serialPorts() const;

    Q_INVOKABLE bool play(const QString& pText);
    Q_INVOKABLE void stop();

    bool            isPlaying() const;
    bool            isAudioRunning() const;
    bool            isKeyDown() const { return m_keyDown; }
    double          decoderSpeed() const;
    QString         decoderPattern() const { return m_decoderPattern; }

signals:
    void            playingChanged();
    void            charPlayed(int pIndex);
    void            audioRunningChanged();
    void            keyDownChanged();
    void            charDecoded(const QString& pText, const QString& pPattern);
    void            wordGap();
    void            decoderSpeedChanged();
    void            decoderPatternChanged();

private slots:
    void            onKeyChanged(bool pDown, qint64 pTimeMs);
    void            restartAudio();
    void            reopenSerial();
    void            applyDecoderMode();

private:
    void            setKeyDown(bool pDown, qint64 pTimeMs);

    Settings*               m_settings;
    AudioEngine             m_audio;
    MorseSequencer          m_sequencer;
    KeyDecoder              m_decoder;
    ManualKeyer             m_manualKeyer;
#ifdef EASYMORSE_HAS_SERIALPORT
    SerialKeyer*            m_serialKeyer;
#endif
    QSet<KeyerInput*>       m_keyers;
    QSet<KeyerInput*>       m_pressed;
    bool                    m_keyDown = false;
    bool                    m_playing = false;
    bool                    m_audioRestartPending = false;
    bool                    m_serialReopenPending = false;
    QString                 m_decoderPattern;
};

#endif // TRAINER_H
