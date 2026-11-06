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
    Q_PROPERTY(double noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY noiseLevelChanged)
    Q_PROPERTY(int noiseFilter READ noiseFilter WRITE setNoiseFilter NOTIFY noiseFilterChanged)
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

public:
    static constexpr int MinFrequency = 200;
    static constexpr int MaxFrequency = 1200;
    static constexpr int MinSpeed = 5;
    static constexpr int MaxSpeed = 40;
    static constexpr int DefaultAudioPeriodMs = 10;

    ///
    /// \param pIniFile settings file, empty for the platform default location
    ///
    explicit Settings(const QString& pIniFile = QString(), QObject* parent = nullptr);
    ~Settings() override;

    int     toneFrequency() const { return m_toneFrequency; }
    int     volume() const { return m_volume; }
    double  noiseLevel() const { return m_noiseLevel; }
    int     noiseFilter() const { return m_noiseFilter; }
    int     wordSpeed() const { return m_wordSpeed; }
    int     charSpeed() const { return m_charSpeed; }
    bool    farnsworth() const { return m_farnsworth; }
    QString audioDevice() const { return m_audioDevice; }
    int     audioPeriodMs() const { return m_audioPeriodMs; }
    bool    serialEnabled() const { return m_serialEnabled; }
    QString serialPort() const { return m_serialPort; }
    bool    decoderAutoAdapt() const { return m_decoderAutoAdapt; }

    void    setToneFrequency(int pHz);
    void    setVolume(int pPercent);
    void    setNoiseLevel(double pLevel);
    void    setNoiseFilter(int pValue);
    void    setWordSpeed(int pWpm);
    void    setCharSpeed(int pWpm);
    void    setFarnsworth(bool pOn);
    void    setAudioDevice(const QString& pName);
    void    setAudioPeriodMs(int pMs);
    void    setSerialEnabled(bool pOn);
    void    setSerialPort(const QString& pName);
    void    setDecoderAutoAdapt(bool pOn);

    ///
    /// \brief decoder estimates learnt in the previous session (0 when unknown)
    ///
    void    decoderEstimates(double& pDotMs, double& pDashMs, double& pGapMs) const;
    void    setDecoderEstimates(double pDotMs, double pDashMs, double pGapMs);

signals:
    void    toneFrequencyChanged();
    void    volumeChanged();
    void    noiseLevelChanged();
    void    noiseFilterChanged();
    void    wordSpeedChanged();
    void    charSpeedChanged();
    void    farnsworthChanged();
    void    audioDeviceChanged();
    void    audioPeriodMsChanged();
    void    serialEnabledChanged();
    void    serialPortChanged();
    void    decoderAutoAdaptChanged();

private:
    template <typename T>
    bool    store(const char* pKey, T& pMember, const T& pValue);

    std::unique_ptr<QSettings>  m_store;
    int         m_toneFrequency;
    int         m_volume;
    double      m_noiseLevel;
    int         m_noiseFilter;
    int         m_wordSpeed;
    int         m_charSpeed;
    bool        m_farnsworth;
    QString     m_audioDevice;
    int         m_audioPeriodMs;
    bool        m_serialEnabled;
    QString     m_serialPort;
    bool        m_decoderAutoAdapt;
};

#endif // SETTINGS_H
