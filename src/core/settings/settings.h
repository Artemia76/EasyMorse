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
* settings.h is part of EasyMorse project
*
* User settings shared by the core and the user interfaces. Each setting is
* a notifiable property, saved at once. Keys are those of the previous
* versions so existing settings are kept.
* **************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDateTime>
#include <QObject>
#include <QString>

#include <memory>

class QSettings;

class Settings : public QObject
{
    Q_OBJECT
    // Sound
    Q_PROPERTY(int toneFrequency READ toneFrequency WRITE setToneFrequency NOTIFY toneFrequencyChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    // Transceiver: back to receive this long after the last key up (0 = QSK)
    Q_PROPERTY(int breakInMs READ breakInMs WRITE setBreakInMs NOTIFY breakInMsChanged)
    // Band conditions: a preset, or Custom once a value is changed by hand
    Q_PROPERTY(int bandPreset READ bandPreset WRITE setBandPreset NOTIFY bandChanged)
    Q_PROPERTY(double noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY bandChanged)
    Q_PROPERTY(int bandwidth READ bandwidth WRITE setBandwidth NOTIFY bandChanged)
    Q_PROPERTY(int qsbDepth READ qsbDepth WRITE setQsbDepth NOTIFY bandChanged)
    Q_PROPERTY(int qsbRate READ qsbRate WRITE setQsbRate NOTIFY bandChanged)
    Q_PROPERTY(int flutter READ flutter WRITE setFlutter NOTIFY bandChanged)
    Q_PROPERTY(int qrnLevel READ qrnLevel WRITE setQrnLevel NOTIFY bandChanged)
    Q_PROPERTY(int qrnRate READ qrnRate WRITE setQrnRate NOTIFY bandChanged)
    Q_PROPERTY(int manMade READ manMade WRITE setManMade NOTIFY bandChanged)
    Q_PROPERTY(bool agc READ agc WRITE setAgc NOTIFY bandChanged)
    Q_PROPERTY(int contestStations READ contestStations WRITE setContestStations NOTIFY bandChanged)
    // Morse speed
    Q_PROPERTY(int wordSpeed READ wordSpeed WRITE setWordSpeed NOTIFY wordSpeedChanged)
    Q_PROPERTY(int charSpeed READ charSpeed WRITE setCharSpeed NOTIFY charSpeedChanged)
    Q_PROPERTY(bool farnsworth READ farnsworth WRITE setFarnsworth NOTIFY farnsworthChanged)
    // Devices
    Q_PROPERTY(QString audioDevice READ audioDevice WRITE setAudioDevice NOTIFY audioDeviceChanged)
    Q_PROPERTY(int audioPeriodMs READ audioPeriodMs WRITE setAudioPeriodMs NOTIFY audioPeriodMsChanged)
    Q_PROPERTY(bool serialEnabled READ serialEnabled WRITE setSerialEnabled NOTIFY serialEnabledChanged)
    Q_PROPERTY(QString serialPort READ serialPort WRITE setSerialPort NOTIFY serialPortChanged)
    // Decoder
    Q_PROPERTY(bool decoderAutoAdapt READ decoderAutoAdapt WRITE setDecoderAutoAdapt NOTIFY decoderAutoAdaptChanged)
    // 0 fast, 1 normal, 2 stable: see KeyDecoder::Reactivity
    Q_PROPERTY(int decoderReactivity READ decoderReactivity WRITE setDecoderReactivity NOTIFY decoderReactivityChanged)
    // Key: 0 straight key, 1 iambic paddles; iambic mode 0 = A, 1 = B
    Q_PROPERTY(int keyerMode READ keyerMode WRITE setKeyerMode NOTIFY keyerModeChanged)
    Q_PROPERTY(int iambicMode READ iambicMode WRITE setIambicMode NOTIFY iambicModeChanged)
    Q_PROPERTY(bool paddlesReversed READ paddlesReversed WRITE setPaddlesReversed NOTIFY paddlesReversedChanged)
    // Keyboard keys: "<Qt key code>:<side>", side 0 = either, 1 = left, 2 = right
    Q_PROPERTY(QString straightKey READ straightKey WRITE setStraightKey NOTIFY keyboardKeysChanged)
    Q_PROPERTY(QString dotKey READ dotKey WRITE setDotKey NOTIFY keyboardKeysChanged)
    Q_PROPERTY(QString dashKey READ dashKey WRITE setDashKey NOTIFY keyboardKeysChanged)
    // Key that ends the capture of the mouse
    Q_PROPERTY(QString releaseKey READ releaseKey WRITE setReleaseKey NOTIFY keyboardKeysChanged)
    // Network
    Q_PROPERTY(QString callsign READ callsign WRITE setCallsign NOTIFY callsignChanged)
    Q_PROPERTY(QString stunServer READ stunServer WRITE setStunServer NOTIFY stunServerChanged)
    Q_PROPERTY(int jitterDelayMs READ jitterDelayMs WRITE setJitterDelayMs NOTIFY jitterDelayMsChanged)
    // What is shown of the received Morse: 0 nothing (listen), 1 after the message, 2 live
    Q_PROPERTY(int receiveDecodeMode READ receiveDecodeMode WRITE setReceiveDecodeMode NOTIFY receiveDecodeModeChanged)
    // New versions: looked for once a day, a version can be skipped
    Q_PROPERTY(bool checkUpdates READ checkUpdates WRITE setCheckUpdates NOTIFY updatesChanged)
    Q_PROPERTY(QString skippedVersion READ skippedVersion WRITE setSkippedVersion NOTIFY updatesChanged)
    Q_PROPERTY(QDateTime lastUpdateCheck READ lastUpdateCheck NOTIFY updatesChanged)
    // Number of characters learnt in the keying practice (Koch method)
    Q_PROPERTY(int kochLevel READ kochLevel WRITE setKochLevel NOTIFY kochLevelChanged)
    // Match-making server
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)

public:
    enum BandPreset
    {
        Clean,          // nothing but the signal
        QuietBand,      // light hiss, slow shallow fading
        Summer40m,      // static crashes of thunderstorms
        Contest,        // stations calling above and below
        Aurora,         // rasping flutter, fast fading
        Urban,          // mains buzz and a stray carrier
        Custom
    };
    Q_ENUM(BandPreset)

    static constexpr int MinFrequency = 200;
    static constexpr int MaxFrequency = 1200;
    static constexpr int MinBandwidth = 100;
    static constexpr int MaxBandwidth = 3000;
    static constexpr int MinQsbRate = 5;            // 1/100 Hz
    static constexpr int MaxQsbRate = 100;
    static constexpr int MaxFlutter = 50;           // Hz
    static constexpr int MaxQrnRate = 60;           // crashes per minute
    static constexpr int MaxContestStations = 8;
    static constexpr int MaxBreakInMs = 2000;
    static constexpr int MinSpeed = 5;
    static constexpr int MaxSpeed = 40;
    static constexpr int DefaultAudioPeriodMs = 10;
    static constexpr int MinKochLevel = 2;
    // The project's match-making server, offered at the first start; its
    // STUN service also finds the public address of manual sessions
    static constexpr const char* DefaultServerUrl = "https://easymorse.f4ikz.ovh";
    static constexpr const char* DefaultStunServer = "easymorse.f4ikz.ovh:3478";

    ///
    /// \param pIniFile settings file, empty for the platform default location
    ///
    explicit Settings(const QString& pIniFile = QString(), QObject* parent = nullptr);
    ~Settings() override;

    int     toneFrequency() const { return m_toneFrequency; }
    int     volume() const { return m_volume; }
    double  noiseLevel() const { return m_noiseLevel; }
    int     breakInMs() const { return m_breakInMs; }
    int     bandPreset() const { return m_bandPreset; }
    ///
    /// \brief width of the receiver filter, in Hz
    ///
    int     bandwidth() const { return m_bandwidth; }
    ///
    /// \brief depth of the random fading of the received signals, in %
    ///
    int     qsbDepth() const { return m_qsbDepth; }
    ///
    /// \brief speed of the fading, in 1/100 Hz
    ///
    int     qsbRate() const { return m_qsbRate; }
    ///
    /// \brief Doppler spread of auroral signals, in Hz (0: none)
    ///
    int     flutter() const { return m_flutter; }
    int     qrnLevel() const { return m_qrnLevel; }
    int     qrnRate() const { return m_qrnRate; }
    int     manMade() const { return m_manMade; }
    bool    agc() const { return m_agc; }
    ///
    /// \brief number of background stations transmitting around the listening frequency
    ///
    int     contestStations() const { return m_contestStations; }
    int     wordSpeed() const { return m_wordSpeed; }
    int     charSpeed() const { return m_charSpeed; }
    bool    farnsworth() const { return m_farnsworth; }
    QString audioDevice() const { return m_audioDevice; }
    int     audioPeriodMs() const { return m_audioPeriodMs; }
    bool    serialEnabled() const { return m_serialEnabled; }
    QString serialPort() const { return m_serialPort; }
    bool    decoderAutoAdapt() const { return m_decoderAutoAdapt; }
    int     decoderReactivity() const { return m_decoderReactivity; }
    int     keyerMode() const { return m_keyerMode; }
    int     iambicMode() const { return m_iambicMode; }
    bool    paddlesReversed() const { return m_paddlesReversed; }
    QString straightKey() const { return m_straightKey; }
    QString dotKey() const { return m_dotKey; }
    QString dashKey() const { return m_dashKey; }
    QString releaseKey() const { return m_releaseKey; }
    QString callsign() const { return m_callsign; }
    QString stunServer() const { return m_stunServer; }
    int     jitterDelayMs() const { return m_jitterDelayMs; }
    int     receiveDecodeMode() const { return m_receiveDecodeMode; }
    bool    checkUpdates() const { return m_checkUpdates; }
    QString skippedVersion() const { return m_skippedVersion; }
    QDateTime lastUpdateCheck() const { return m_lastUpdateCheck; }
    int     kochLevel() const { return m_kochLevel; }
    QString serverUrl() const { return m_serverUrl; }
    ///
    /// \brief token of the last login, to reconnect without the password
    ///
    QString serverToken() const { return m_serverToken; }

    void    setToneFrequency(int pHz);
    void    setVolume(int pPercent);
    void    setNoiseLevel(double pLevel);
    ///
    /// \brief setBandPreset applies the values of a preset (nothing for Custom)
    ///
    void    setBandPreset(int pPreset);
    void    setBreakInMs(int pMs);
    void    setBandwidth(int pHz);
    void    setQsbDepth(int pPercent);
    void    setQsbRate(int pCentiHz);
    void    setFlutter(int pHz);
    void    setQrnLevel(int pPercent);
    void    setQrnRate(int pPerMinute);
    void    setManMade(int pPercent);
    void    setAgc(bool pOn);
    void    setContestStations(int pCount);
    void    setWordSpeed(int pWpm);
    void    setCharSpeed(int pWpm);
    void    setFarnsworth(bool pOn);
    void    setAudioDevice(const QString& pName);
    void    setAudioPeriodMs(int pMs);
    void    setSerialEnabled(bool pOn);
    void    setSerialPort(const QString& pName);
    void    setDecoderAutoAdapt(bool pOn);
    void    setDecoderReactivity(int pReactivity);
    void    setKeyerMode(int pMode);
    void    setIambicMode(int pMode);
    void    setPaddlesReversed(bool pReversed);
    void    setStraightKey(const QString& pKey);
    void    setDotKey(const QString& pKey);
    void    setDashKey(const QString& pKey);
    void    setReleaseKey(const QString& pKey);
    void    setCallsign(const QString& pCallsign);
    void    setStunServer(const QString& pServer);
    void    setJitterDelayMs(int pMs);
    void    setReceiveDecodeMode(int pMode);
    void    setCheckUpdates(bool pCheck);
    void    setSkippedVersion(const QString& pVersion);
    void    setLastUpdateCheck(const QDateTime& pTime);
    void    setKochLevel(int pLevel);
    void    setServerUrl(const QString& pUrl);
    void    setServerToken(const QString& pToken);

    ///
    /// \brief decoder estimates learnt in the previous session (0 when unknown)
    ///
    void    decoderEstimates(double& pDotMs, double& pDashMs, double& pGapMs) const;
    void    setDecoderEstimates(double pDotMs, double pDashMs, double pGapMs);

signals:
    void    toneFrequencyChanged();
    void    volumeChanged();
    void    bandChanged();
    void    breakInMsChanged();
    void    wordSpeedChanged();
    void    charSpeedChanged();
    void    farnsworthChanged();
    void    audioDeviceChanged();
    void    audioPeriodMsChanged();
    void    serialEnabledChanged();
    void    serialPortChanged();
    void    decoderAutoAdaptChanged();
    void    decoderReactivityChanged();
    void    keyerModeChanged();
    void    iambicModeChanged();
    void    paddlesReversedChanged();
    void    keyboardKeysChanged();
    void    callsignChanged();
    void    stunServerChanged();
    void    jitterDelayMsChanged();
    void    receiveDecodeModeChanged();
    void    updatesChanged();
    void    kochLevelChanged();
    void    serverUrlChanged();

private:
    template <typename T>
    bool    store(const char* pKey, T& pMember, const T& pValue);
    ///
    /// \brief setBandValue changes a band condition by hand: the preset becomes Custom
    ///
    template <typename T>
    void    setBandValue(const char* pKey, T& pMember, const T& pValue);

    std::unique_ptr<QSettings>  m_store;
    int         m_toneFrequency;
    int         m_volume;
    double      m_noiseLevel;
    int         m_breakInMs;
    int         m_bandPreset;
    int         m_bandwidth;
    int         m_qsbDepth;
    int         m_qsbRate;
    int         m_flutter;
    int         m_qrnLevel;
    int         m_qrnRate;
    int         m_manMade;
    bool        m_agc;
    int         m_contestStations;
    int         m_wordSpeed;
    int         m_charSpeed;
    bool        m_farnsworth;
    QString     m_audioDevice;
    int         m_audioPeriodMs;
    bool        m_serialEnabled;
    QString     m_serialPort;
    bool        m_decoderAutoAdapt;
    int         m_decoderReactivity;
    int         m_keyerMode;
    int         m_iambicMode;
    bool        m_paddlesReversed;
    QString     m_straightKey;
    QString     m_dotKey;
    QString     m_dashKey;
    QString     m_releaseKey;
    QString     m_callsign;
    QString     m_stunServer;
    int         m_jitterDelayMs;
    int         m_receiveDecodeMode;
    bool        m_checkUpdates;
    QString     m_skippedVersion;
    QDateTime   m_lastUpdateCheck;
    int         m_kochLevel;
    QString     m_serverUrl;
    QString     m_serverToken;
};

#endif // SETTINGS_H
