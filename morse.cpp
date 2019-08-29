#include "morse.h"

morse::morse(QObject *parent) : QObject(parent)
{
    m_generator.reset(new Generateur());
    m_generator->setLoop(false);
    m_dotDuration = 100000;
    initMorseMap(International);
}
void morse::initMorseMap (Code pCode)
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
    m_MorseMapping[" "] = " ";
}

void morse::code(const QString& pMessage)
{
    m_generator->clear();
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
                m_generator->generateData(m_dotDuration*3, false, true );
            }
        }
        // End of char
        m_generator->generateData(m_dotDuration*2, false, true );
    }
    m_generator->start();
}

Generateur* morse::data()
{
    return m_generator.data();
}
