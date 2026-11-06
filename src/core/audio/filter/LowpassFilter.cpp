/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "LowpassFilter.h"

//ctor
LowpassFilter::LowpassFilter()
    : m_dCutoffFrequency(0.999)
    , m_dBuf0(0.0)
    , m_dBuf1(0.0)
    , m_dBuf2(0.0)
    , m_dResonance(0.0)
    , m_bNoteOn(false)
{
  calculateFeedbackAmount();
}

//dtor
LowpassFilter::~LowpassFilter()
{

}

void LowpassFilter::triggerOn()
{
  m_bNoteOn = true;
}

void LowpassFilter::triggerOff()
{
  // reset buffers
  m_bNoteOn = false;
}

double LowpassFilter::process(double dInputValue)
{
  m_dBuf0 += m_dCutoffFrequency *  (dInputValue - m_dBuf0 + m_dFeedbackAmount * (m_dBuf0 - m_dBuf1));
  m_dBuf1 += m_dCutoffFrequency *  (m_dBuf0 - m_dBuf1);
  m_dBuf2 += m_dCutoffFrequency *  (m_dBuf1 - m_dBuf2);
  
  return m_dBuf2;
}

void LowpassFilter::reset()
{
  m_dBuf0 = m_dBuf1 = m_dBuf2 = 0.0;
}

void LowpassFilter::setCutoffFrequency(double dFrequency)
{
  m_dCutoffFrequency = dFrequency;
  calculateFeedbackAmount();
}

void LowpassFilter::setResonance(double dResonance)
{
  m_dResonance = dResonance;
  calculateFeedbackAmount();
}

double LowpassFilter::getFeedbackAmount()
{
  return m_dFeedbackAmount;
}
