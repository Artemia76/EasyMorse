/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
** Contact: https://github.com/Artemia76/EasyMorse
**
** EasyMorse is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** EasyMorse is distributed in the hope that it will be useful,
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
* morsecodec.cpp is part of EasyMorse project
*
* International Morse code table
* **************************************************************************/

#include "morsecodec.h"

#include <QHash>

#include <utility>

namespace
{
    struct Tables
    {
        QList<QPair<QChar, QString>>    entries;
        QHash<QChar, QString>           encode;
        QHash<QString, QChar>           decode;

        Tables()
        {
            entries = {
                {u'A', ".-"},     {u'B', "-..."},   {u'C', "-.-."},   {u'D', "-.."},
                {u'E', "."},      {u'F', "..-."},   {u'G', "--."},    {u'H', "...."},
                {u'I', ".."},     {u'J', ".---"},   {u'K', "-.-"},    {u'L', ".-.."},
                {u'M', "--"},     {u'N', "-."},     {u'O', "---"},    {u'P', ".--."},
                {u'Q', "--.-"},   {u'R', ".-."},    {u'S', "..."},    {u'T', "-"},
                {u'U', "..-"},    {u'V', "...-"},   {u'W', ".--"},    {u'X', "-..-"},
                {u'Y', "-.--"},   {u'Z', "--.."},
                {u'1', ".----"},  {u'2', "..---"},  {u'3', "...--"},  {u'4', "....-"},
                {u'5', "....."},  {u'6', "-...."},  {u'7', "--..."},  {u'8', "---.."},
                {u'9', "----."},  {u'0', "-----"},
                {u'.', ".-.-.-"}, {u',', "--..--"}, {u'?', "..--.."}, {u'\'', ".----."},
                {u'!', "-.-.--"}, {u'/', "-..-."},  {u'(', "-.--."},  {u')', "-.--.-"},
                {u'&', ".-..."},  {u':', "---..."}, {u';', "-.-.-."}, {u'=', "-...-"},
                {u'+', ".-.-."},  {u'-', "-....-"}, {u'_', "..--.-"}, {u'"', ".-..-."},
                {u'$', "...-..-"},{u'@', ".--.-."},
                {u'Ä', ".-.-"},   {u'À', ".--.-"},  {u'Ç', "-.-.."},  {u'È', ".-..-"},
                {u'É', "..-.."},
            };
            for (const auto& entry : std::as_const(entries))
            {
                encode.insert(entry.first, entry.second);
                // First character wins when several share a pattern
                if (!decode.contains(entry.second))
                    decode.insert(entry.second, entry.first);
            }
        }
    };

    const Tables& tables()
    {
        static const Tables instance;
        return instance;
    }
}

QString MorseCodec::encode(QChar pChar)
{
    return tables().encode.value(pChar.toUpper());
}

QString MorseCodec::decode(const QString& pPattern)
{
    const auto it = tables().decode.constFind(pPattern);
    return it == tables().decode.constEnd() ? QString() : QString(*it);
}

const QList<QPair<QChar, QString>>& MorseCodec::table()
{
    return tables().entries;
}
