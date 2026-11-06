/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#ifndef IPARAM_H
#define IPARAM_H

class Param
{
public:

    Param(double dMinValue, double dMaxValue, double dDefaultValue);

    double getValue();
    void setValue(double);

    float getMinValue();
    void setMinValue(double dMinValue);
    
    float getMaxValue();
    void setMaxValue(double dMaxValue);

protected:

    double   m_dValue;
    double   m_dMinValue;
    double   m_dMaxValue;
};

#endif
