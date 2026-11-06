/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "BandpassFilter.h"

//ctor
BandpassFilter::BandpassFilter()
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
BandpassFilter::~BandpassFilter()
{

}

void BandpassFilter::triggerOn()
{
  m_bNoteOn = true;
}

void BandpassFilter::triggerOff()
{
  // reset buffers
  m_bNoteOn = false;
}

double BandpassFilter::process(double dInputValue)
{
  m_dBuf0 += m_dCutoffFrequency *  (dInputValue - m_dBuf0 + m_dFeedbackAmount * (m_dBuf0 - m_dBuf1));
  m_dBuf1 += m_dCutoffFrequency *  (m_dBuf0 - m_dBuf1);
  m_dBuf2 += m_dCutoffFrequency *  (m_dBuf1 - m_dBuf2);
  
  return m_dBuf0 - m_dBuf2;
}

void BandpassFilter::reset()
{
  m_dBuf0 = m_dBuf1 = m_dBuf2 = 0.0;
}

void BandpassFilter::setCutoffFrequency(double dFrequency)
{
  m_dCutoffFrequency = dFrequency;
  calculateFeedbackAmount();
}

void BandpassFilter::setResonance(double dResonance)
{
  m_dResonance = dResonance;
  calculateFeedbackAmount();
}

double BandpassFilter::getFeedbackAmount()
{
  return m_dFeedbackAmount;
}
