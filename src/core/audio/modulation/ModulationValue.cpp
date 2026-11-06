/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "ModulationValue.h"

#include <math.h>

namespace modulation
{
    ModulationValue::ModulationValue(double dMinValue, double dMaxValue, double dDefaultValue)
    : Param(dMinValue, dMaxValue, dDefaultValue)
    , m_bActivated(true)
    {

    }

    double ModulationValue::getModulatedValue(double dTime)
    {
        if(m_bActivated)
        {
            double amplitude = 1.0;
            if (m_pEnvelopeGenerator)
            {
                amplitude = m_pEnvelopeGenerator->getAmplitude(dTime);
            }

            return fmax(fmin( amplitude * (m_dValue + m_pLfo->getModulationAmount()), m_dMaxValue), m_dMinValue);
        }

        return getValue();
    }

    void ModulationValue::setLfo(std::shared_ptr<ILfo> pModulator)
    {
        m_pLfo = pModulator;
    }

    void ModulationValue::setEnvelopeModulator(std::shared_ptr<IEnvelope> pEnvelopeModulator)
    {
        m_pEnvelopeGenerator = pEnvelopeModulator;
    }
}
