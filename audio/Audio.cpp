/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "Audio.h"

#include <cmath>

/*static*/
int AudioHal::m_sDefaultDeviceNumber = -1;


//ctor
AudioHal::AudioHal()
: m_frequency(0.0)
, m_globalTime(0.0)
, stream(0)
, left_phase(0)
, right_phase(0)
, m_Playing(false)
{
    m_log = CLogger::instance();
}

QStringList AudioHal::getAudioDeviceList()
{
    QStringList Liste;
    int numDevices;
    numDevices = Pa_GetDeviceCount();
    for (int i=0; i < numDevices; i++)
    {
        Liste.append(Pa_GetDeviceInfo( i )->name);
        m_log->log( QString("Device %1").arg(i) + QString::fromStdString(Pa_GetDeviceInfo( i )->name),Qt::darkGreen,LEVEL_NORMAL);
    }
    return Liste;
}

void AudioHal::initialize()
{
    m_log->log("AudioHal: Initializing audio interface", Qt::darkGreen,LEVEL_NORMAL);
    
    PaError err;

    err = Pa_Initialize();

    if (err != paNoError)
    {
        m_log->log("AudioHal: Error while initialiazing", Qt::red);
    }
    
    const PaDeviceInfo *deviceInfo;

    m_log->log("AudioHal: Setting audio to default device",Qt::darkGreen,LEVEL_NORMAL);
    m_sDefaultDeviceNumber = Pa_GetDefaultOutputDevice();

    m_log->log( "Default Output: [ " ,Qt::darkGreen,LEVEL_NORMAL);

    deviceInfo = Pa_GetDeviceInfo( m_sDefaultDeviceNumber );

    if( m_sDefaultDeviceNumber == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultOutputDevice )
    {
        const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
        m_log->log(hostInfo->name, Qt::darkGreen,LEVEL_NORMAL);
    }

    m_log->log(" ]",Qt::darkGreen,LEVEL_NORMAL);
}

void AudioHal::terminate()
{
    Pa_Terminate();
}

void AudioHal::open()
{
    PaStreamParameters outputParameters;
    PaError err;


    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
      m_log->log("Error: No default output device", Qt::red);
      terminate();
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    //outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.suggestedLatency = 0.050;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    printf("AudioHal:: Opening stream...\n");
    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paNoFlag,//paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              &AudioHal::paCallback,
              this );


    if (err != paNoError)
    {
        terminate();
        return;
    }

    err = Pa_StartStream( stream );
    
    if (err != paNoError)
    {
        return;
    }
}

void AudioHal::play(double frequency)
{
    m_frequency = frequency;
    
    if (m_pVoiceManager != nullptr)
    {   
        m_log->log("AudioHal::play",Qt::darkGreen,LEVEL_VERBOSE);
        m_pVoiceManager->noteOn(frequency, getGlobalTime());
    }
 }

void AudioHal::stop(double frequency)
{
    if (m_pVoiceManager)
    {
        m_log->log("AudioHal::stop",Qt::darkGreen,LEVEL_VERBOSE);
        m_pVoiceManager->noteOff(frequency, getGlobalTime());
    }
}

void AudioHal::startNoise()
{
    if (m_pVoiceManager != nullptr)
    {
        m_log->log("AudioHal::start noise",Qt::darkGreen,LEVEL_VERBOSE);
        m_pVoiceManager->noiseOn(getGlobalTime());
    }
 }

void AudioHal::stopNoise()
{
    if (m_pVoiceManager)
    {
        m_log->log("AudioHal::stop noise",Qt::darkGreen,LEVEL_VERBOSE);
        m_pVoiceManager->noiseOff(getGlobalTime());
    }
}

int AudioHal::paUserCallback(   const void *inputBuffer, void *outputBuffer,
                                unsigned long /*framesPerBuffer*/,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags)
{
    float *out = (float*)outputBuffer;
    int j;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;
    
    //create signal
    for( j=0; j<TABLE_SIZE; j++ )
    {
        float value = static_cast<float>(m_pVoiceManager->getSample(m_globalTime));

        *out++ = value;  /* left */
        *out++ = value;  /* right */

        m_globalTime += TIME_STEP;
    }

    return paContinue;
}

/*
SETTER AND GETTER
*/

double AudioHal::getGlobalTime()
{
    return m_globalTime;
}

void AudioHal::setVoiceManager(VoiceManager* pVoicemanager)
{
    m_pVoiceManager = pVoicemanager;
}
