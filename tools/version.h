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

#ifndef VERSION_H
#define VERSION_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VER_FILEVERSION             APP_MAJOR,APP_MINOR,APP_BUILD,0
#define VER_FILEVERSION_STR         STR(APP_MAJOR) "." STR(APP_MINOR) "." STR(APP_BUILD) ".0\0"

#define VER_PRODUCTVERSION          APP_MAJOR,APP_MINOR,APP_BUILD,0
#define VER_PRODUCTVERSION_STR      STR(APP_MAJOR) "." STR(APP_MINOR) "." STR(APP_BUILD)

#define VER_COMPANYNAME_STR         "SHTSF"
#define VER_FILEDESCRIPTION_STR     "EasyMorse"
#define VER_INTERNALNAME_STR        "EasyMorse"
#define VER_LEGALCOPYRIGHT_STR      "Copyright © 2019 Gianni Peschiutta"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_LEGALTRADEMARKS2_STR    VER_LEGALTRADEMARKS1_STR
#define VER_ORIGINALFILENAME_STR    "easymorse.exe"
#define VER_PRODUCTNAME_STR         "EasyMorse"

#define VER_COMPANYDOMAIN_STR       "shtsf.com"

#endif
