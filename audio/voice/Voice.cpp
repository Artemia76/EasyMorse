/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "Voice.h"
#include "audio/filter/HighpassFilter.h"
#include "audio/filter/LowpassFilter.h"
#include "audio/filter/BandpassFilter.h"

#include <iostream>

Voice::Voice( std::shared_ptr<IOscillatorFunction> pOscillator1
            , std::shared_ptr<IOscillatorFunction> pOscillator2
            , std::shared_ptr<IFilter> pFilter
            , std::shared_ptr<ILfo> pLfo
            , std::shared_ptr<IEnvelope> pFilterEnvelope
            , std::shared_ptr<IEnvelope> pAmpEnvelope)
    : m_pOscillator1(pOscillator1)
    , m_pOscillator2(pOscillator2)
    , m_pFilter(pFilter)
    , m_pAmpEnvelope(pAmpEnvelope)
    , m_pFilterEnvelope(pFilterEnvelope)
    , m_dFrequency(0.0)
    , m_dMix(0.5)
    , m_bActive(false)
{   
    m_pFilterCutOff = std::make_shared<modulation::ModulationValue>(0.01, 0.99, 0.99);
    m_pFilterCutOff->setLfo(pLfo);
    m_pFilterCutOff->setEnvelopeModulator(m_pFilterEnvelope);

    m_pFilterResonance = std::make_shared<modulation::ModulationValue>(0.01, 0.99, 0.01);
    m_pFilterResonance->setLfo(pLfo);
    m_pFilterResonance->setEnvelopeModulator(m_pFilterEnvelope);
}

Voice::~Voice()
{

}

void Voice::noteOn(double dFrequency, double dTime)
{
    m_bActive = true;
    m_dFrequency = dFrequency;
    m_pAmpEnvelope->noteOn(dTime);
    m_pFilterEnvelope->noteOn(dTime);
    m_pFilter->triggerOn();
}

void Voice::noteOff(double dTime)
{   
    m_pAmpEnvelope->noteOff(dTime);
    m_pFilterEnvelope->noteOff(dTime);
    m_pFilter->triggerOff();
}

double Voice::process(double dTime)
{
    double osc1 = m_pOscillator1->calculate(m_dFrequency, dTime);
    double osc2 = m_pOscillator2->calculate(m_dFrequency, dTime);
    double amp = m_pAmpEnvelope->getAmplitude(dTime);
    
    m_pFilter->setCutoffFrequency(m_pFilterCutOff->getModulatedValue(dTime));
    m_pFilter->setResonance(m_pFilterResonance->getModulatedValue(dTime));
    double filtered = m_pFilter->process( amp * (((1.0 - m_dMix) * osc1 + m_dMix * osc2)));
    
    if ((m_pAmpEnvelope->isNoteOff() && m_pAmpEnvelope->getCurrentAmplitude() == 0.0))
    {
        m_bActive = false;
    }
    
    return filtered;
}

void Voice::reset()
{
    m_pAmpEnvelope->reset();
    m_pFilterEnvelope->reset();
    m_pFilter->reset();
}

bool Voice::isActive()
{
    return m_bActive;
}

void Voice::setActive()
{
    m_bActive = true;
}

/******************************************************************************
    Setters and Getters
******************************************************************************/

double Voice::getFrequency()
{
    return m_dFrequency;
}

std::shared_ptr<IOscillatorFunction> Voice::getOscillator1()
{
    return m_pOscillator1;
}

void Voice::setOscillator1(std::shared_ptr<IOscillatorFunction> pOscillator)
{
    m_pOscillator1 = pOscillator;
}
    
std::shared_ptr<IOscillatorFunction> Voice::getOscillator2()
{
    return m_pOscillator2;
}

void Voice::setOscillator2(std::shared_ptr<IOscillatorFunction> pOscillator)
{
    m_pOscillator2 = pOscillator;
}

std::shared_ptr<IEnvelope> Voice::getAmpEnvelope()
{
    return m_pAmpEnvelope;
}

std::shared_ptr<IEnvelope> Voice::getFilterEnvelope()
{
    return m_pFilterEnvelope;
}
    
std::shared_ptr<IFilter> Voice::getFilter()
{
    return m_pFilter;
}

void Voice::setFilter(FilterType type)
{
    switch (type)
    {
        case FilterType::LOWPASS:
            m_pFilter = std::make_shared<LowpassFilter>();
        break;
        case FilterType::HIGHPASS:
            m_pFilter = std::make_shared<HighpassFilter>();
        break;
        case FilterType::BANDPASS:
            m_pFilter = std::make_shared<BandpassFilter>();
        break;
    }
}

void Voice::setOscillationMix(double dMix)
{
    m_dMix = dMix;
}

std::shared_ptr<modulation::ModulationValue> Voice::getFilterCutOff()
{
    return m_pFilterCutOff;
}

std::shared_ptr<modulation::ModulationValue> Voice::getFilterResonance()
{
    return m_pFilterResonance;
}
