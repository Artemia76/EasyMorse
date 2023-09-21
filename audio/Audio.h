/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

#include <iostream>
#include <map>
#include <cmath>

#include <QObject>

//#include "IOscillatorFunction.h"
#include <portaudio.h>

#include "audio/voicemanager/VoiceManager.h"
#include "tools/clogger.h"

#define SAMPLE_RATE (44100.0)
#define TABLE_SIZE (128)
#define FRAMES_PER_BUFFER TABLE_SIZE
#define TIME_STEP (1.0/SAMPLE_RATE)

class AudioHal : public QObject
{
public:

    AudioHal();

    void initialize();
    void terminate();
    void open();
    void play(double frequency);
    void stop(double frequency);
    void startNoise();
    void stopNoise();

    int paUserCallback( const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags);

    double getGlobalTime();

    void setVoiceManager(VoiceManager* pVoicemanager);

    void setVolume(int Volume);

    /*STATIC*/
    static int paCallback( const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData )
    {
        return ((AudioHal*)userData)->paUserCallback(inputBuffer, outputBuffer,
            framesPerBuffer,
            timeInfo,
            statusFlags);
    }

     
private:

    static int m_sDefaultDeviceNumber;

    double m_frequency;
    double m_globalTime;

    VoiceManager* m_pVoiceManager;
    CLogger* m_log;

    PaStream *stream;
    int left_phase;
    int right_phase;

    bool m_Playing;
};

#endif
