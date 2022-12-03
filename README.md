#EasyMorse build instruction

Easy Morse is QT5 based application to learn Morse code by earing and produce. In future version it will could used in association with SDR or RADIO to decode and produce morse code.

EasyMorse will be designed on open source project model and will be writted in C++ with portable library (Qt)

EasyMorse is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

EasyMorse is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

The license is as published by the Free Software
Foundation and appearing in the file LICENSE.GPL3
included in the packaging of this software. Please review the following
information to ensure the GNU General Public License requirements will
be met: [GPLv3 licence ](https://www.gnu.org/licenses/gpl-3.0.html)

1. DEPENDENCIES

	- Qt : >=5.9

1. Build on Linux
	1. UBUNTU

		On ubuntu you can enter this command line how provide you all dependencies needed to build ffs2play:

			sudo apt-get install build-essential qt5-default pkg-config

	1. Gentoo / Calculate Linux

		Prepare dependencies on terminal as superuser:

			emerge @qt5-essentials

	1. Arch Linux

		Prepare dependencies on terminal :

			sudo yaourt -S base-devel gdb qt5-base

	1. Final build:

		on git root :

			qmake CONFIG+=release
			make -j3
			./EasyMorse

	1. Update

		repeat previous commands after git pull

1. WIN(Visual studio community edition)

	coming ...
