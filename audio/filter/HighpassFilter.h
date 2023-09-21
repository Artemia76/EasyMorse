/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#ifndef HIGHPASS_FILTER_H
#define HIGHPASS_FILTER_H

#include "IFilter.h"

class HighpassFilter : public IFilter
{
public:
    //ctor
    HighpassFilter();
    //dtor
    ~HighpassFilter();

    void triggerOn() override;
    void triggerOff() override;

    double process(double dInputValue) override;
    void  reset() override;

    void setCutoffFrequency(double dFrequency) override;
    void setResonance(double dResonance) override;
    double getFeedbackAmount() override;

private:

    double m_dCutoffFrequency;
    double m_dBuf0;
    double m_dBuf1;
    double m_dBuf2;

    double m_dResonance;
    double m_dFeedbackAmount;

    bool m_bNoteOn;

    inline void calculateFeedbackAmount() { m_dFeedbackAmount = m_dResonance + m_dResonance/(1.0 - m_dCutoffFrequency); }
};

#endif
