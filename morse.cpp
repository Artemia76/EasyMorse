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
    m_generator.reset(new CGenerator());
    m_generator->setLoop(false);
    m_dotDuration = 100000;
    m_frequency = m_generator->getFrequency();
    m_noiseFilter=0;
    m_noiseCorrelation=0.0;
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
    m_generator->stop();
    m_generator->clear();
    m_generator->setFrequency(m_frequency);
    m_generator->setNoiseCorrelation(m_noiseCorrelation);
    m_generator->setNoiseFilter(m_noiseFilter);
    for (QString::const_iterator Char = pMessage.begin(); Char!=pMessage.end(); ++Char)
    {
        QString Morse = m_MorseMapping[Char->toUpper()];
        for (QString::const_iterator MorseChar = Morse.begin(); MorseChar!=Morse.end(); ++MorseChar)
        {
            if (*MorseChar=='P')
            {
                m_generator->generateData(m_dotDuration );
                m_generator->generateData(m_dotDuration, false, true );
            }
            if (*MorseChar=='T')
            {
                m_generator->generateData(m_dotDuration * 3);
                m_generator->generateData(m_dotDuration, false, true );
            }
            if (*MorseChar=='S')
            {
                m_generator->generateData(m_dotDuration*4, false, true );
            }
        }
        // End of char
        m_generator->generateData(m_dotDuration*2, false, true );
    }
    m_generator->start();
}

CGenerator* CMorse::data()
{
    return m_generator.data();
}
