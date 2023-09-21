/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#ifndef IFILTER_H
#define IFILTER_H

class IFilter
{

public: 
    virtual void triggerOn() = 0;
    virtual void triggerOff() = 0;

    virtual double process(double dInputValue) = 0;
    virtual void reset() = 0;


    virtual void setCutoffFrequency(double dFrequency) = 0;
    virtual void setResonance(double dResonance) = 0;
    virtual double getFeedbackAmount() = 0;
};

#endif
