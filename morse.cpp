/****************************************************************************
**
** Copyright (C) 2019 Gianni Peschiutta
** Contact: https://bitbucket.org/Artemia/easymorse/src/master/
**
** FFS2Play is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** FFS2Play is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** The license is as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

/****************************************************************************
* morse.cpp is part of EasyMorse project
*
* This class is used to code and decode morse signal
* **************************************************************************/

#include "morse.h"

CMorse::CMorse(QObject *parent) : QObject(parent)
{
    m_generator.setLoop(false);
    m_wordSpeed = 12;
    m_charSpeed = 18;
    m_frequency = m_generator.getFrequency();
    m_noiseFilter=0;
    m_noiseCorrelation=0.0;
    m_farnsWorth=1.0;
    initMorseMap(International);
}
void CMorse::initMorseMap (Code pCode)
{
    m_MorseMapping.clear();
    switch (pCode)
    {
    case International:
        m_MorseMapping["A"] = "PT";
        m_MorseMapping["B"] = "TPPP";
        m_MorseMapping["C"] = "TPTP";
        m_MorseMapping["D"] = "TPP";
        m_MorseMapping["E"] = "P";
        m_MorseMapping["F"] = "PPTP";
        m_MorseMapping["G"] = "TTP";
        m_MorseMapping["H"] = "PPPP";
        m_MorseMapping["I"] = "PP";
        m_MorseMapping["J"] = "PTTT";
        m_MorseMapping["K"] = "TPT";
        m_MorseMapping["L"] = "PTPP";
        m_MorseMapping["M"] = "TT";
        m_MorseMapping["N"] = "TP";
        m_MorseMapping["O"] = "TTT";
        m_MorseMapping["P"] = "PTTP";
        m_MorseMapping["Q"] = "TTPT";
        m_MorseMapping["R"] = "PTP";
        m_MorseMapping["S"] = "PPP";
        m_MorseMapping["T"] = "T";
        m_MorseMapping["U"] = "PPT";
        m_MorseMapping["V"] = "PPPT";
        m_MorseMapping["W"] = "PTT";
        m_MorseMapping["X"] = "TPPT";
        m_MorseMapping["Y"] = "TPTT";
        m_MorseMapping["Z"] = "TTPP";
        m_MorseMapping["1"] = "PTTTT";
        m_MorseMapping["2"] = "PPTTT";
        m_MorseMapping["3"] = "PPPTT";
        m_MorseMapping["4"] = "PPPPT";
        m_MorseMapping["5"] = "PPPPP";
        m_MorseMapping["6"] = "TPPPP";
        m_MorseMapping["7"] = "TTPPP";
        m_MorseMapping["8"] = "TTTPP";
        m_MorseMapping["9"] = "TTTTP";
        m_MorseMapping["0"] = "TTTTT";
        break;
    case American:
        break;
    case Continental:
        break;
    }
    m_MorseMapping[" "] = "S";
}

void CMorse::code(const QString& pMessage)
{
    m_generator.stop();
    m_generator.clear();
    m_generator.setFrequency(m_frequency);
    m_generator.setNoiseCorrelation(m_noiseCorrelation);
    m_generator.setNoiseFilter(m_noiseFilter);
    quint64 DotDuration;
    quint64 DelayChar;
    quint64 DelayWord;
    DotDuration = qRound(60000/static_cast<double>(m_wordSpeed*50))*1000;
    DelayChar = DotDuration*2;
    DelayWord =DotDuration*4;
    if (m_farnsWorth)
    {
        if (m_wordSpeed < m_charSpeed)
        {
            DotDuration = qRound(60000/static_cast<double>(m_charSpeed*50))*1000;
            quint64 ta = ((m_charSpeed * 60000000) - (37200000 * m_wordSpeed)) / ( m_charSpeed * m_wordSpeed);
            DelayChar = (3 * ta)/19;
            DelayWord = (7 * ta)/19;
        }
    }

    m_generator.generateData(DelayChar, false, true ); // Silent before start
    for (QString::const_iterator Char = pMessage.begin(); Char!=pMessage.end(); ++Char)
    {
        QString Morse = m_MorseMapping[Char->toUpper()];
        for (QString::const_iterator MorseChar = Morse.begin(); MorseChar!=Morse.end(); ++MorseChar)
        {
            if (*MorseChar=='P')
            {
                m_generator.generateData(DotDuration ); //Make a dot
                m_generator.generateData(DotDuration, false, true ); // Silent between element
            }
            if (*MorseChar=='T')
            {
                m_generator.generateData(DotDuration * 3); // Make a dash
                m_generator.generateData(DotDuration, false, true ); // Silent between element
            }
            if (*MorseChar=='S')
            {
                m_generator.generateData(DelayWord, false, true ); // Silent between Word
            }
        }
        // End of char
        m_generator.generateData(DelayChar, false, true ); //Silent between Char
    }
    m_generator.generateData(DelayChar, false, true ); // Silent at end
    m_generator.start();
}

CGenerator* CMorse::data()
{
    return &m_generator;
}

void CMorse::setFarnsWorth(bool pFW)
{
    m_farnsWorth=pFW;
}

bool CMorse::getFarnsWorth()
{
    return m_farnsWorth;
}
