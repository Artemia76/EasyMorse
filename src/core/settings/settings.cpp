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
* settings.cpp is part of EasyMorse project
* **************************************************************************/

#include "settings.h"

#include <QSettings>

namespace Key
{
    constexpr const char* ToneFrequency = "Synth/SoundFreq";
    constexpr const char* Volume = "Synth/SoundLevel";
    constexpr const char* NoiseLevel = "Synth/NoiseCorrelation";
    constexpr const char* BandPreset = "Band/Preset";
    constexpr const char* Bandwidth = "Band/Bandwidth";
    constexpr const char* QsbDepth = "Band/QsbDepth";
    constexpr const char* QsbRate = "Band/QsbRate";
    constexpr const char* Flutter = "Band/Flutter";
    constexpr const char* QrnLevel = "Band/QrnLevel";
    constexpr const char* QrnRate = "Band/QrnRate";
    constexpr const char* ManMade = "Band/ManMade";
    constexpr const char* Agc = "Band/Agc";
    constexpr const char* ContestStations = "Band/ContestStations";
    constexpr const char* WordSpeed = "Morse/WordSpeed";
    constexpr const char* CharSpeed = "Morse/CharSpeed";
    constexpr const char* Farnsworth = "Morse/FarnsWorth";
    constexpr const char* AudioDevice = "audio/DeviceName";
    constexpr const char* AudioPeriodMs = "audio/PeriodMs";
    constexpr const char* SerialEnabled = "serial/PortEnable";
    constexpr const char* SerialPort = "serial/PortName";
    constexpr const char* DecoderAutoAdapt = "decoder/AutoAdapt";
    constexpr const char* DecoderReactivity = "decoder/Reactivity";
    constexpr const char* KeyerMode = "keyer/Mode";
    constexpr const char* IambicMode = "keyer/IambicMode";
    constexpr const char* PaddlesReversed = "keyer/PaddlesReversed";
    constexpr const char* StraightKey = "keyer/StraightKey";
    constexpr const char* DotKey = "keyer/DotKey";
    constexpr const char* DashKey = "keyer/DashKey";
    constexpr const char* Callsign = "network/Callsign";
    constexpr const char* StunServer = "network/StunServer";
    constexpr const char* JitterDelayMs = "network/JitterDelayMs";
    constexpr const char* ReceiveDecodeMode = "network/ReceiveDecodeMode";
    constexpr const char* ServerUrl = "server/Url";
    constexpr const char* ServerToken = "server/Token";
    constexpr const char* DecoderDotMs = "decoder/DotMs";
    constexpr const char* DecoderDashMs = "decoder/DashMs";
    constexpr const char* DecoderGapMs = "decoder/GapMs";
}

Settings::Settings(const QString& pIniFile, QObject* parent)
    : QObject(parent)
    , m_store(pIniFile.isEmpty() ? std::make_unique<QSettings>()
                                 : std::make_unique<QSettings>(pIniFile, QSettings::IniFormat))
{
    m_toneFrequency = qBound(MinFrequency, m_store->value(Key::ToneFrequency, 900).toInt(), MaxFrequency);
    m_volume = qBound(0, m_store->value(Key::Volume, 100).toInt(), 100);
    m_noiseLevel = qBound(0.0, m_store->value(Key::NoiseLevel, 0.0).toDouble(), 1.0);
    // Settings of a version without presets: keep the noise chosen then
    m_bandPreset = qBound(0, m_store->value(Key::BandPreset, m_noiseLevel > 0.0 ? Custom : Clean).toInt(), int(Custom));
    m_bandwidth = qBound(MinBandwidth, m_store->value(Key::Bandwidth, 500).toInt(), MaxBandwidth);
    m_qsbDepth = qBound(0, m_store->value(Key::QsbDepth, 0).toInt(), 100);
    m_qsbRate = qBound(MinQsbRate, m_store->value(Key::QsbRate, 10).toInt(), MaxQsbRate);
    m_flutter = qBound(0, m_store->value(Key::Flutter, 0).toInt(), MaxFlutter);
    m_qrnLevel = qBound(0, m_store->value(Key::QrnLevel, 0).toInt(), 100);
    m_qrnRate = qBound(0, m_store->value(Key::QrnRate, 0).toInt(), MaxQrnRate);
    m_manMade = qBound(0, m_store->value(Key::ManMade, 0).toInt(), 100);
    m_agc = m_store->value(Key::Agc, false).toBool();
    m_contestStations = qBound(0, m_store->value(Key::ContestStations, 0).toInt(), MaxContestStations);
    // A preset always sounds as in this version, even if its values were tuned since
    if (m_bandPreset != Custom)
        setBandPreset(m_bandPreset);
    m_wordSpeed = qBound(MinSpeed, m_store->value(Key::WordSpeed, 12).toInt(), MaxSpeed);
    m_charSpeed = qBound(MinSpeed, m_store->value(Key::CharSpeed, 18).toInt(), MaxSpeed);
    m_farnsworth = m_store->value(Key::Farnsworth, true).toBool();
    m_audioDevice = m_store->value(Key::AudioDevice).toString();
    m_audioPeriodMs = qBound(2, m_store->value(Key::AudioPeriodMs, DefaultAudioPeriodMs).toInt(), 200);
    m_serialEnabled = m_store->value(Key::SerialEnabled, false).toBool();
    m_serialPort = m_store->value(Key::SerialPort).toString();
    m_decoderAutoAdapt = m_store->value(Key::DecoderAutoAdapt, true).toBool();
    m_decoderReactivity = qBound(0, m_store->value(Key::DecoderReactivity, 1).toInt(), 2);
    m_keyerMode = qBound(0, m_store->value(Key::KeyerMode, 0).toInt(), 1);
    m_iambicMode = qBound(0, m_store->value(Key::IambicMode, 1).toInt(), 1);
    m_paddlesReversed = m_store->value(Key::PaddlesReversed, false).toBool();
    // Left Alt as in the previous versions (right Alt is AltGr on many
    // layouts, needed to type); paddles for a right-handed operator:
    // left Ctrl for dots and left Alt for dashes, side by side
#ifdef Q_OS_MACOS
    const int control = Qt::Key_Meta;   // the Control key on macOS
#else
    const int control = Qt::Key_Control;
#endif
    m_straightKey = m_store->value(Key::StraightKey, QString("%1:1").arg(int(Qt::Key_Alt))).toString();
    m_dotKey = m_store->value(Key::DotKey, QString("%1:1").arg(control)).toString();
    m_dashKey = m_store->value(Key::DashKey, QString("%1:1").arg(int(Qt::Key_Alt))).toString();
    m_callsign = m_store->value(Key::Callsign).toString();
    m_stunServer = m_store->value(Key::StunServer, DefaultStunServer).toString();
    m_jitterDelayMs = qBound(0, m_store->value(Key::JitterDelayMs, 200).toInt(), 2000);
    m_receiveDecodeMode = qBound(0, m_store->value(Key::ReceiveDecodeMode, 0).toInt(), 2);
    m_serverUrl = m_store->value(Key::ServerUrl, DefaultServerUrl).toString();
    m_serverToken = m_store->value(Key::ServerToken).toString();
}

Settings::~Settings() = default;

template <typename T>
bool Settings::store(const char* pKey, T& pMember, const T& pValue)
{
    if (pMember == pValue)
        return false;
    pMember = pValue;
    m_store->setValue(pKey, pValue);
    return true;
}

template <typename T>
void Settings::setBandValue(const char* pKey, T& pMember, const T& pValue)
{
    if (!store(pKey, pMember, pValue))
        return;
    store(Key::BandPreset, m_bandPreset, int(Custom));
    emit bandChanged();
}

namespace
{
    struct Conditions
    {
        double  noiseLevel;
        int     bandwidth;
        int     qsbDepth;
        int     qsbRate;        // 1/100 Hz
        int     flutter;        // Hz
        int     qrnLevel;
        int     qrnRate;        // per minute
        int     manMade;
        bool    agc;
        int     contestStations;
    };

    // In the order of Settings::BandPreset
    constexpr Conditions Presets[] = {
        // noise  BW   QSB rate flutter QRN rate man AGC   stations
        {0.00,   500,   0, 10,  0,   0,  0,   0, false, 0},   // Clean
        {0.15,   500,  30,  8,  0,   0,  0,   0, true,  0},   // Quiet band
        {0.35,   500,  50, 15,  0,  50, 30,   0, true,  1},   // 40 m in summer
        {0.25,   500,  40, 12,  0,  10,  6,   0, true,  6},   // Contest
        {0.30,   500,  60, 50, 25,   0,  0,   0, true,  0},   // Aurora
        {0.50,   500,  20, 10,  0,   0,  0,  50, true,  0},   // Urban
    };
}

void Settings::setBandPreset(int pPreset)
{
    pPreset = qBound(0, pPreset, int(Custom));
    bool changed = store(Key::BandPreset, m_bandPreset, pPreset);
    if (pPreset != Custom)
    {
        const Conditions& c = Presets[pPreset];
        changed |= store(Key::NoiseLevel, m_noiseLevel, c.noiseLevel);
        changed |= store(Key::Bandwidth, m_bandwidth, c.bandwidth);
        changed |= store(Key::QsbDepth, m_qsbDepth, c.qsbDepth);
        changed |= store(Key::QsbRate, m_qsbRate, c.qsbRate);
        changed |= store(Key::Flutter, m_flutter, c.flutter);
        changed |= store(Key::QrnLevel, m_qrnLevel, c.qrnLevel);
        changed |= store(Key::QrnRate, m_qrnRate, c.qrnRate);
        changed |= store(Key::ManMade, m_manMade, c.manMade);
        changed |= store(Key::Agc, m_agc, c.agc);
        changed |= store(Key::ContestStations, m_contestStations, c.contestStations);
    }
    if (changed)
        emit bandChanged();
}

void Settings::setToneFrequency(int pHz)
{
    if (store(Key::ToneFrequency, m_toneFrequency, qBound(MinFrequency, pHz, MaxFrequency)))
        emit toneFrequencyChanged();
}

void Settings::setVolume(int pPercent)
{
    if (store(Key::Volume, m_volume, qBound(0, pPercent, 100)))
        emit volumeChanged();
}

void Settings::setNoiseLevel(double pLevel)
{
    setBandValue(Key::NoiseLevel, m_noiseLevel, qBound(0.0, pLevel, 1.0));
}

void Settings::setBandwidth(int pHz)
{
    setBandValue(Key::Bandwidth, m_bandwidth, qBound(MinBandwidth, pHz, MaxBandwidth));
}

void Settings::setQsbDepth(int pPercent)
{
    setBandValue(Key::QsbDepth, m_qsbDepth, qBound(0, pPercent, 100));
}

void Settings::setQsbRate(int pCentiHz)
{
    setBandValue(Key::QsbRate, m_qsbRate, qBound(MinQsbRate, pCentiHz, MaxQsbRate));
}

void Settings::setFlutter(int pHz)
{
    setBandValue(Key::Flutter, m_flutter, qBound(0, pHz, MaxFlutter));
}

void Settings::setQrnLevel(int pPercent)
{
    setBandValue(Key::QrnLevel, m_qrnLevel, qBound(0, pPercent, 100));
}

void Settings::setQrnRate(int pPerMinute)
{
    setBandValue(Key::QrnRate, m_qrnRate, qBound(0, pPerMinute, MaxQrnRate));
}

void Settings::setManMade(int pPercent)
{
    setBandValue(Key::ManMade, m_manMade, qBound(0, pPercent, 100));
}

void Settings::setAgc(bool pOn)
{
    setBandValue(Key::Agc, m_agc, pOn);
}

void Settings::setContestStations(int pCount)
{
    setBandValue(Key::ContestStations, m_contestStations, qBound(0, pCount, MaxContestStations));
}

void Settings::setWordSpeed(int pWpm)
{
    if (store(Key::WordSpeed, m_wordSpeed, qBound(MinSpeed, pWpm, MaxSpeed)))
        emit wordSpeedChanged();
}

void Settings::setCharSpeed(int pWpm)
{
    if (store(Key::CharSpeed, m_charSpeed, qBound(MinSpeed, pWpm, MaxSpeed)))
        emit charSpeedChanged();
}

void Settings::setFarnsworth(bool pOn)
{
    if (store(Key::Farnsworth, m_farnsworth, pOn))
        emit farnsworthChanged();
}

void Settings::setAudioDevice(const QString& pName)
{
    if (store(Key::AudioDevice, m_audioDevice, pName))
        emit audioDeviceChanged();
}

void Settings::setAudioPeriodMs(int pMs)
{
    if (store(Key::AudioPeriodMs, m_audioPeriodMs, qBound(2, pMs, 200)))
        emit audioPeriodMsChanged();
}

void Settings::setSerialEnabled(bool pOn)
{
    if (store(Key::SerialEnabled, m_serialEnabled, pOn))
        emit serialEnabledChanged();
}

void Settings::setSerialPort(const QString& pName)
{
    if (store(Key::SerialPort, m_serialPort, pName))
        emit serialPortChanged();
}

void Settings::setDecoderAutoAdapt(bool pOn)
{
    if (store(Key::DecoderAutoAdapt, m_decoderAutoAdapt, pOn))
        emit decoderAutoAdaptChanged();
}

void Settings::setDecoderReactivity(int pReactivity)
{
    if (store(Key::DecoderReactivity, m_decoderReactivity, qBound(0, pReactivity, 2)))
        emit decoderReactivityChanged();
}

void Settings::setKeyerMode(int pMode)
{
    if (store(Key::KeyerMode, m_keyerMode, qBound(0, pMode, 1)))
        emit keyerModeChanged();
}

void Settings::setIambicMode(int pMode)
{
    if (store(Key::IambicMode, m_iambicMode, qBound(0, pMode, 1)))
        emit iambicModeChanged();
}

void Settings::setPaddlesReversed(bool pReversed)
{
    if (store(Key::PaddlesReversed, m_paddlesReversed, pReversed))
        emit paddlesReversedChanged();
}

void Settings::setStraightKey(const QString& pKey)
{
    if (store(Key::StraightKey, m_straightKey, pKey))
        emit keyboardKeysChanged();
}

void Settings::setDotKey(const QString& pKey)
{
    if (store(Key::DotKey, m_dotKey, pKey))
        emit keyboardKeysChanged();
}

void Settings::setDashKey(const QString& pKey)
{
    if (store(Key::DashKey, m_dashKey, pKey))
        emit keyboardKeysChanged();
}

void Settings::setCallsign(const QString& pCallsign)
{
    if (store(Key::Callsign, m_callsign, pCallsign.trimmed().toUpper().left(16)))
        emit callsignChanged();
}

void Settings::setStunServer(const QString& pServer)
{
    if (store(Key::StunServer, m_stunServer, pServer.trimmed()))
        emit stunServerChanged();
}

void Settings::setJitterDelayMs(int pMs)
{
    if (store(Key::JitterDelayMs, m_jitterDelayMs, qBound(0, pMs, 2000)))
        emit jitterDelayMsChanged();
}

void Settings::setReceiveDecodeMode(int pMode)
{
    if (store(Key::ReceiveDecodeMode, m_receiveDecodeMode, qBound(0, pMode, 2)))
        emit receiveDecodeModeChanged();
}

void Settings::setServerUrl(const QString& pUrl)
{
    QString url = pUrl.trimmed();
    // A bare host name means HTTPS
    if (!url.isEmpty() && !url.contains("://"))
        url.prepend("https://");
    while (url.endsWith(u'/'))
        url.chop(1);
    if (store(Key::ServerUrl, m_serverUrl, url))
        emit serverUrlChanged();
}

void Settings::setServerToken(const QString& pToken)
{
    store(Key::ServerToken, m_serverToken, pToken);
}

void Settings::decoderEstimates(double& pDotMs, double& pDashMs, double& pGapMs) const
{
    pDotMs = m_store->value(Key::DecoderDotMs, 0.0).toDouble();
    pDashMs = m_store->value(Key::DecoderDashMs, 0.0).toDouble();
    pGapMs = m_store->value(Key::DecoderGapMs, 0.0).toDouble();
}

void Settings::setDecoderEstimates(double pDotMs, double pDashMs, double pGapMs)
{
    m_store->setValue(Key::DecoderDotMs, pDotMs);
    m_store->setValue(Key::DecoderDashMs, pDashMs);
    m_store->setValue(Key::DecoderGapMs, pGapMs);
}
