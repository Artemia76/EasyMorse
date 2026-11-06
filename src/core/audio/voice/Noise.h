#ifndef NOISE_H
#define NOISE_H

#include "INoise.h"
#include <QRandomGenerator>

class Noise : public INoise
{
public:
    // ctor
    Noise(std::shared_ptr<IFilter> pFilter
        , std::shared_ptr<ILfo> pLfo
        , std::shared_ptr<IEnvelope> pFilterEnvelope);
    //dtor
    ~Noise();

    void noteOn(double dTime) override;

    void noteOff(double dTime) override;

    double process(double dTime) override;

    void reset() override;

    bool isActive() override;
    void setActive() override;

    std::shared_ptr<modulation::ModulationValue> getFilterCutOff() override;
    std::shared_ptr<modulation::ModulationValue> getFilterResonance() override;

private:

    std::shared_ptr<IFilter> m_pFilter;
    std::shared_ptr<IEnvelope> m_pFilterEnvelope;
    bool m_bActive;
    QRandomGenerator m_Rnd;

/******************************************************************************
    Modulation
******************************************************************************/
    std::shared_ptr<modulation::ModulationValue> m_pFilterCutOff;
    std::shared_ptr<modulation::ModulationValue> m_pFilterResonance;

/******************************************************************************
    Setters and Getters
******************************************************************************/

    std::shared_ptr<IFilter> getFilter() override;
    void setFilter(FilterType type) override;
};

#endif // NOISE_H
