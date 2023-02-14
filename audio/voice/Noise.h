#ifndef NOISE_H
#define NOISE_H

#include "INoise.h"

#include "audio/voicemanager/Param.h"

class Noise : public INoise
{
public:
    // ctor
    Noise(std::shared_ptr<IFilter> pFilter
        , std::shared_ptr<ILfo> pLfo
        , std::shared_ptr<IEnvelope> pFilterEnvelope);
    //dtor
    ~Noise();

    void noteOn(float fTime) override;

    void noteOff(float fTime) override;

    float process(float fTime) override;

    void reset() override;

    bool isActive() override;
    void setActive() override;

    std::shared_ptr<modulation::ModulationValue> getFilterCutOff();
    std::shared_ptr<modulation::ModulationValue> getFilterResonance();

private:

    std::shared_ptr<IFilter> m_pFilter;
    std::shared_ptr<IEnvelope> m_pFilterEnvelope;
    bool m_bActive;

/******************************************************************************
    Modulation
******************************************************************************/
    std::shared_ptr<modulation::ModulationValue> m_pFilterCutOff;
    std::shared_ptr<modulation::ModulationValue> m_pFilterResonance;

/******************************************************************************
    Setters and Getters
******************************************************************************/

    std::shared_ptr<IFilter> getFilter() override;
    void setFilter(FilterType type);
};

#endif // NOISE_H
