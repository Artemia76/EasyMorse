#include "Noise.h"
#include "audio/filter/HighpassFilter.h"
#include "audio/filter/LowpassFilter.h"
#include "audio/filter/BandpassFilter.h"
#include <iostream>

Noise::Noise(std::shared_ptr<IFilter> pFilter
            , std::shared_ptr<ILfo> pLfo
            , std::shared_ptr<IEnvelope> pFilterEnvelope)
: m_pFilter(pFilter)
, m_bActive(false)
, m_pFilterEnvelope(pFilterEnvelope)
{
    m_pFilterCutOff = std::make_shared<modulation::ModulationValue>(0.01, 0.99, 0.99);
    m_pFilterCutOff->setLfo(pLfo);
    m_pFilterCutOff->setEnvelopeModulator(m_pFilterEnvelope);

    m_pFilterResonance = std::make_shared<modulation::ModulationValue>(0.01, 0.99, 0.01);
    m_pFilterResonance->setLfo(pLfo);
    m_pFilterResonance->setEnvelopeModulator(m_pFilterEnvelope);
}

Noise::~Noise()
{

}

void Noise::noteOn(float fTime)
{
    m_bActive = true;
    m_pFilterEnvelope->noteOn(fTime);
    m_pFilter->triggerOn();
}

void Noise::noteOff(float fTime)
{
    m_bActive = false;
    m_pFilterEnvelope->noteOff(fTime);
    m_pFilter->triggerOff();
}

float Noise::process(float fTime)
{

    m_pFilter->setCutoffFrequency(m_pFilterCutOff->getModulatedValue(fTime));
    m_pFilter->setResonance(m_pFilterResonance->getModulatedValue(fTime));
    float filtered = m_pFilter->process((float) m_Rnd.generateDouble()* 2.0 - 1.0);

    return filtered;
}

void Noise::reset()
{
    m_pFilter->reset();
}

bool Noise::isActive()
{
    return m_bActive;
}

void Noise::setActive()
{
    m_bActive = true;
}

/******************************************************************************
    Setters and Getters
******************************************************************************/

std::shared_ptr<IFilter> Noise::getFilter()
{
    return m_pFilter;
}

void Noise::setFilter(FilterType type)
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

std::shared_ptr<modulation::ModulationValue> Noise::getFilterCutOff()
{
    return m_pFilterCutOff;
}

std::shared_ptr<modulation::ModulationValue> Noise::getFilterResonance()
{
    return m_pFilterResonance;
}
