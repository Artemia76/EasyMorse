/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "Envelope.h"

Envelope::Envelope( std::shared_ptr<Param> pAttackTime
    	, std::shared_ptr<Param> pDecayTime
    	, std::shared_ptr<Param> pReleaseTime
    	, std::shared_ptr<Param> pSustainAmplitude)
: m_pAttackTime(pAttackTime)
, m_pDecayTime(pDecayTime)
, m_pReleaseTime(pReleaseTime)
, m_pSustainAmplitude(pSustainAmplitude)
, m_dActualSustainAmplitude(0.0)
, m_dStartAmplitude(1.0)
, m_dTriggerOffTime(0.0)
, m_dTriggerOnTime(0.0)
, m_dCurrentAmplitude(0.0)
, m_bNoteOn(false)

{
}

Envelope::~Envelope()
{

}

void Envelope::noteOn(double dTime)
{
    m_dActualSustainAmplitude = 0.0;
    m_dTriggerOnTime = dTime;
	m_bNoteOn = true;
}

void Envelope::noteOff(double dTime)
{
    m_dTriggerOffTime = dTime;
	m_bNoteOn = false;
}

double Envelope::getAmplitude(double dTime)
{
    double dAmplitude = 0.0;
    double dLifeTime = 0.0;

	if (m_bNoteOn)
	{
        dLifeTime = dTime - m_dTriggerOnTime;

        if (dLifeTime <= m_pAttackTime->getValue())
		{
			// In attack Phase - approach max amplitude
            dAmplitude = (dLifeTime / m_pAttackTime->getValue()) * m_dStartAmplitude;
		}

        else if (dLifeTime > m_pAttackTime->getValue() && dLifeTime <= (m_pAttackTime->getValue() + m_pDecayTime->getValue()))
		{
			// In decay phase - reduce to sustained amplitude
            dAmplitude = ((dLifeTime - m_pAttackTime->getValue()) / m_pDecayTime->getValue()) * (m_pSustainAmplitude->getValue() - m_dStartAmplitude) + m_dStartAmplitude;
		}

        else if (dLifeTime > (m_pAttackTime->getValue() + m_pDecayTime->getValue()))
		{
			// In sustain phase - dont change until note released
            dAmplitude = m_pSustainAmplitude->getValue();
		}
        m_dActualSustainAmplitude = dAmplitude;
	}
	else
	{
        dLifeTime = dTime - m_dTriggerOffTime;

		// Note has been released, so in release phase
        dAmplitude = ((dLifeTime) / m_pReleaseTime->getValue()) * (0.0 - m_dActualSustainAmplitude) + m_dActualSustainAmplitude;
	}

	// Amplitude should not be negative
    if (dAmplitude <= 0.0001)
        dAmplitude = 0.0;

    m_dCurrentAmplitude = dAmplitude;

    return dAmplitude;
}

double Envelope::getCurrentAmplitude()
{
    return m_dCurrentAmplitude;
}

void Envelope::reset()
{
	m_bNoteOn = false;
    m_dCurrentAmplitude = 0.0;
    m_dTriggerOffTime = 0.0;
    m_dTriggerOnTime = 0.0;
}

bool Envelope::isNoteOff()
{
	return (!m_bNoteOn);
}
