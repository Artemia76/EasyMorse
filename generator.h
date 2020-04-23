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
** generator.h is part of EasyMorse project
**
** This class provide way to synthesize wave
** form signal to compose morse tone
****************************************************************************/

#ifndef GENERATEUR_H
#define GENERATEUR_H

#include <QAudioOutput>
#include <QRandomGenerator>
#include <QByteArray>
#include <QIODevice>
#include <QObject>
#include <QMap>

#include "clogger.h"

class CGenerator : public QIODevice
{
public:
    CGenerator();
    CGenerator(int pFrequency);

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    qint64 pos() const override;
    bool atEnd() const override;

    void setFormat(const QAudioFormat &pFormat);
    QAudioFormat getFormat();
    void setFrequency(int pFreq);
    int getFrequency();
    void setNoiseCorrelation(qreal pNoiseCorrelation);
    void setNoiseFilter(int pNoiseFilter);
    void setLoop(bool pLoop);
    void generateData(qint64 durationUs=1000000,bool pErase=false, bool pSilent=false);
    void clear();
private:
    qint64 m_pos = 0;
    bool m_loop;
    QByteArray m_buffer;
static QAudioFormat m_Format;
    int m_Freq;
    QRandomGenerator m_Rnd;
    qreal   m_noise_correlation;
    qreal   m_Lowfilter_T;
    qreal   m_LastSample;
    CLogger* m_log;
    void init();
};

#endif // GENERATEUR_H
