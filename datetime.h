/****************************************************************************
**
** Copyright (C) 2017 FSFranceSimulateur team.
** Contact: https://github.com/ffs2/ffs2play
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
** datetime.h is part of FF2Play project
**
** These types and function provide time convertion between timestamp format
** and std::chrono tools
****************************************************************************/

#ifndef DATETIME_H
#define DATETIME_H

#include <chrono>

using namespace std::chrono;

using datetime = time_point<system_clock,milliseconds>;

using timespan = milliseconds;

datetime Now();

datetime fromTimeStamp(uint64_t pTS);
uint64_t toTimeStamp(datetime pTime);

#endif
