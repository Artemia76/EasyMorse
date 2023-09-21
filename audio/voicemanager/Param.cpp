/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#include "Param.h"
#include "qdebug.h"

Param::Param(double dMinValue, double dMaxValue, double dDefaultValue)
    : m_dValue(dDefaultValue)
    , m_dMinValue(dMinValue)
    , m_dMaxValue(dMaxValue)
{

}

double Param::getValue()
{
    return m_dValue;
}

void Param::setValue(double dValue)
{
    if (dValue <= m_dMaxValue && dValue >= m_dMinValue)
    {
        qDebug() << "Setting value to " << dValue;
        m_dValue = dValue;
    }
}

void Param::setMinValue(double dMinValue)
{
    m_dMinValue = dMinValue;
}

float Param::getMinValue()
{
    return m_dMinValue;
}

void Param::setMaxValue(double dMaxValue)
{
    m_dMaxValue = dMaxValue;
}

float Param::getMaxValue()
{
    return m_dMaxValue;
}
