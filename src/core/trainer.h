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
#include <QTimer>

#include <map>
#include <memory>

#include "audio/AudioEngine.h"
#include "keyer/iambickeyer.h"
#include "keyer/keyerinput.h"
#include "morse/keydecoder.h"
#include "morse/sequencer.h"
#include "net/lobby.h"
#include "net/peersession.h"

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
    // Paddles of the touch screen, and the contacts closed on any paddle
    Q_PROPERTY(Paddles* manualPaddles READ manualPaddles CONSTANT)
    Q_PROPERTY(bool dotPaddle READ isDotPaddle NOTIFY paddlesChanged)
    Q_PROPERTY(bool dashPaddle READ isDashPaddle NOTIFY paddlesChanged)
    // Remote station
    Q_PROPERTY(PeerSession* session READ session CONSTANT)
    Q_PROPERTY(Lobby* lobby READ lobby CONSTANT)
    Q_PROPERTY(bool remoteKeyDown READ isRemoteKeyDown NOTIFY remoteKeyDownChanged)
    Q_PROPERTY(bool remoteActive READ isRemoteActive NOTIFY remoteActiveChanged)
    Q_PROPERTY(double remoteDecoderSpeed READ remoteDecoderSpeed NOTIFY remoteDecoderSpeedChanged)
    Q_PROPERTY(QString remoteCallsign READ remoteCallsign NOTIFY remoteDecoderSpeedChanged)

public:
    explicit Trainer(Settings* pSettings, QObject* parent = nullptr);
    ~Trainer() override;

    ///
    /// \brief start opens the sound card and the serial key chosen in the settings
    ///
    void            start();
    ///
    /// \brief suspend closes the sound output while the application is in
    /// the background (phones); resume opens it again
    ///
    void            suspend();
    void            resume();

    Settings*       settings() const { return m_settings; }
    AudioEngine*    audio() { return &m_audio; }
    KeyDecoder*     decoder() { return &m_decoder; }
    ManualKeyer*    manualKeyer() { return &m_manualKeyer; }
    Paddles*        manualPaddles() { return &m_manualPaddles; }
    PeerSession*    session() { return &m_session; }
    Lobby*          lobby() { return &m_lobby; }

    ///
    /// \brief addKeyer listens to another key (remote station...)
    ///
    void            addKeyer(KeyerInput* pKeyer);
    void            removeKeyer(KeyerInput* pKeyer);
    ///
    /// \brief addPaddles listens to the contacts of a paddle (used in iambic mode)
    ///
    void            addPaddles(Paddles* pPaddles);
    bool            isDotPaddle() const { return m_dotPaddle; }
    bool            isDashPaddle() const { return m_dashPaddle; }

    Q_INVOKABLE QStringList audioDevices();
    Q_INVOKABLE QStringList serialPorts() const;

    Q_INVOKABLE bool play(const QString& pText);
    Q_INVOKABLE void stop();
    ///
    /// \brief speed learnt by the decoder of a remote station, in WPM
    ///
    Q_INVOKABLE double stationSpeed(PeerSession* pSession) const;

    bool            isPlaying() const;
    bool            isAudioRunning() const;
    bool            isKeyDown() const { return m_keyDown; }
    double          decoderSpeed() const;
    QString         decoderPattern() const { return m_decoderPattern; }
    bool            isRemoteKeyDown() const;
    ///
    /// \brief the remote station keyed recently: its message is not over
    ///
    bool            isRemoteActive() const { return m_remoteActive; }
    ///
    /// \brief speed of the station heard last
    ///
    double          remoteDecoderSpeed() const;
    QString         remoteCallsign() const { return m_remoteCallsign; }

signals:
    void            playingChanged();
    void            charPlayed(int pIndex);
    ///
    /// \brief characters of the played text as they are sent (with the spaces before them)
    ///
    void            charSent(const QString& pText);
    void            audioRunningChanged();
    void            keyDownChanged();
    void            paddlesChanged();
    void            charDecoded(const QString& pText, const QString& pPattern);
    void            wordGap();
    void            decoderSpeedChanged();
    void            decoderPatternChanged();
    void            remoteKeyDownChanged();
    void            remoteActiveChanged();
    void            remoteCharDecoded(PeerSession* pSession, const QString& pText, const QString& pPattern);
    void            remoteWordGap(PeerSession* pSession);
    void            remoteSpeedChanged(PeerSession* pSession);
    void            remoteDecoderSpeedChanged();

private slots:
    void            onKeyChanged(bool pDown, qint64 pTimeMs);
    void            applySessionSettings();
    void            restartAudio();
    void            reopenSerial();
    void            applyDecoderMode();
    void            applyKeyerMode();
    void            onPaddlesChanged(qint64 pTimeMs);

private:
    void            setKeyDown(bool pDown, qint64 pTimeMs);
    bool            isAnyKeyDown() const;
    void            sendKey(bool pDown, qint64 pTimeMs);
    ///
    /// \brief send the key changes of the played text when they are due
    ///
    void            sendPlayedKeys();
    void            endPlayedKeys();
    ///
    /// \brief play and decode a remote station (manual session or room member)
    ///
    void            addRemote(PeerSession* pSession);
    void            removeRemote(PeerSession* pSession);
    void            onRemoteKeyChanged(PeerSession* pSession, bool pDown, qint64 pTimeMs);
    void            silenceRemote(PeerSession* pSession);
    QString         remoteName(PeerSession* pSession) const;

    Settings*               m_settings;
    AudioEngine             m_audio;
    MorseSequencer          m_sequencer;
    KeyDecoder              m_decoder;
    ManualKeyer             m_manualKeyer;
    Paddles                 m_manualPaddles;
    IambicKeyer             m_iambic;
    QList<Paddles*>         m_paddles;
    bool                    m_dotPaddle = false;
    bool                    m_dashPaddle = false;
    PeerSession             m_session;
    Lobby                   m_lobby;
    // One decoder per remote station: each one's speed is learnt separately
    std::map<PeerSession*, std::unique_ptr<KeyDecoder>> m_remoteDecoders;
    QSet<PeerSession*>      m_remoteDown;
    PeerSession*            m_lastRemote = nullptr;
    QString                 m_remoteCallsign;
    QTimer                  m_remoteIdleTimer;
    bool                    m_remoteActive = false;
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
    // Played text, sent to the network with its exact rhythm
    QString                 m_playText;
    int                     m_playShown = -1;
    QList<TimelineEvent>    m_playEdges;
    qsizetype               m_playNext = 0;
    qint64                  m_playStartMs = 0;
    bool                    m_playKeyDown = false;
    QTimer                  m_playKeyTimer;
};

#endif // TRAINER_H
