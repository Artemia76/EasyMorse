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
    constexpr const char* NoiseFilter = "Synth/NoiseFilter";
    constexpr const char* WordSpeed = "Morse/WordSpeed";
    constexpr const char* CharSpeed = "Morse/CharSpeed";
    constexpr const char* Farnsworth = "Morse/FarnsWorth";
    constexpr const char* AudioDevice = "audio/DeviceName";
    constexpr const char* AudioPeriodMs = "audio/PeriodMs";
    constexpr const char* SerialEnabled = "serial/PortEnable";
    constexpr const char* SerialPort = "serial/PortName";
    constexpr const char* DecoderAutoAdapt = "decoder/AutoAdapt";
    constexpr const char* DecoderReactivity = "decoder/Reactivity";
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
    m_noiseFilter = qMax(0, m_store->value(Key::NoiseFilter, 0).toInt());
    m_wordSpeed = qBound(MinSpeed, m_store->value(Key::WordSpeed, 12).toInt(), MaxSpeed);
    m_charSpeed = qBound(MinSpeed, m_store->value(Key::CharSpeed, 18).toInt(), MaxSpeed);
    m_farnsworth = m_store->value(Key::Farnsworth, true).toBool();
    m_audioDevice = m_store->value(Key::AudioDevice).toString();
    m_audioPeriodMs = qBound(2, m_store->value(Key::AudioPeriodMs, DefaultAudioPeriodMs).toInt(), 200);
    m_serialEnabled = m_store->value(Key::SerialEnabled, false).toBool();
    m_serialPort = m_store->value(Key::SerialPort).toString();
    m_decoderAutoAdapt = m_store->value(Key::DecoderAutoAdapt, true).toBool();
    m_decoderReactivity = qBound(0, m_store->value(Key::DecoderReactivity, 1).toInt(), 2);
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
    if (store(Key::NoiseLevel, m_noiseLevel, qBound(0.0, pLevel, 1.0)))
        emit noiseLevelChanged();
}

void Settings::setNoiseFilter(int pValue)
{
    if (store(Key::NoiseFilter, m_noiseFilter, qMax(0, pValue)))
        emit noiseFilterChanged();
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
