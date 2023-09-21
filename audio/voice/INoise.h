/**
 * @copyright Copyright (c) 2023 Gianni Peschiutta
 * @author Gianni Peschiutta
 * @brief  This file is part of EasyMorse (Morse Learning Software)
 * @date 2023-02-14
 */

#ifndef INOISE_H
#define INOISE_H

#include "audio/filter/IFilter.h"
#include "audio/modulation/ModulationValue.h"
#include "audio/filter/FilterTypes.h"


#include <memory>

class INoise
{
public:

    virtual void noteOn(double dTime) = 0;

    virtual void noteOff(double dTime) = 0;

    virtual double process(double dTime) = 0;

    virtual void reset() = 0;

    virtual bool isActive() = 0;
    virtual void setActive() = 0;

    virtual std::shared_ptr<IFilter> getFilter() = 0;
    virtual void setFilter(FilterType type) = 0;

    virtual std::shared_ptr<modulation::ModulationValue> getFilterCutOff() = 0;
    virtual std::shared_ptr<modulation::ModulationValue> getFilterResonance() = 0;
};

#endif // INOISE_H
