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
* morse.h is part of EasyMorse project
*
* This class is used to code and decode morse signal
* **************************************************************************/

#ifndef MORSE_H
#define MORSE_H

#include <QObject>
#include <QMap>

#include "sound/generator.h"

class CMorse : public QObject
{
    Q_OBJECT
public:
    explicit CMorse(QObject *parent = nullptr );
    enum Code
    {
        American,
        Continental,
        International
    };
    Q_ENUM(Code)

    void                        initMorseMap (Code pCode=International);
    void                        code(const QString& pMessage);
    CGenerator*                 data();
    void                        setFormat(const QAudioFormat &pFormat);
    void                        setFrequency(int pFrequency){m_frequency=pFrequency;}
    void                        setWordSpeed(int pWordSpeed){m_wordSpeed=pWordSpeed;}
    void                        setCharSpeed(int pCharSpeed){m_charSpeed=pCharSpeed;}
    void                        setNoiseCorrelation(qreal pNoiseCorrelation){m_noiseCorrelation=pNoiseCorrelation;}
    void                        setNoiseFilter(int pNoiseFilter){m_noiseFilter=pNoiseFilter;}
    void                        setFarnsWorth(bool pFW);
    bool                        getFarnsWorth();
    QString                     decodeMorse(QString pMessage);

private:
    int                         m_frequency;
    bool                        m_farnsWorth;
    qreal                       m_noiseCorrelation;
    int                         m_noiseFilter;
    QMap                        <QString, QString> m_MorseMapping;
    quint32                     m_wordSpeed;
    quint32                     m_charSpeed;
    CGenerator                  m_generator;
    QAudioFormat                m_Format;
signals:

public slots:
};

#endif // MORSE_H
