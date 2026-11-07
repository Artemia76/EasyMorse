# EasyMorse

EasyMorse is a Qt 6 application to learn Morse code by ear and by hand:
play any text in Morse (with Farnsworth timing, background noise and filter),
key Morse yourself with a straight key or iambic paddles, and watch it being
decoded in real time, whatever your speed. Practise with other operators over
the Internet (see below). Android/iOS support is planned.

## Keys

Choose the keying on the *Key* page or in the options:

- **Straight key**: the Alt key, the on-screen key (mouse or touch) or a key
  wired to a serial port.
- **Iambic paddles** (modes A and B, reversible for left-handed operators): the
  keyer sends dots and dashes at the character speed. Left Ctrl is the dot
  paddle and right Ctrl the dash paddle, as USB paddle adapters (VBand) send
  them; on-screen paddles can be held together on a touch screen.

Serial port wiring, as usual for Morse software: DTR and RTS are kept active to
feed the contacts. A straight key closes CTS or DSR; paddles close CTS or DCD
for dots and DSR for dashes.

## License

EasyMorse is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

EasyMorse is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details: [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html)
and the `LICENSE.GPLv3` file.

The audio synthesizer is derived from
[qt-port-synth](https://github.com/rluedek/qt-port-synth) by Rico Luedeke.

## Dependencies

- CMake >= 3.21 (3.27+ to build the Windows Inno Setup installer) and Ninja
- A C++20 compiler (GCC 11+, Clang 14+, MSVC 2022)
- Qt >= 6.7: Qt Base, Qt Declarative (Qt Quick, Quick Controls), Qt Linguist tools,
  and optionally Qt Serial Port for a serial key
- Audio uses [miniaudio](https://miniaud.io), bundled in `3rdparty/`: nothing to install
- Network sessions use [libjuice](https://github.com/paullouisageneau/libjuice) (installed, e.g.
  `libjuice` on Arch, or downloaded and built automatically) and the bundled Monocypher

### Arch Linux

    sudo pacman -S base-devel cmake ninja qt6-base qt6-declarative qt6-serialport qt6-tools \
        qt6-websockets qt6-svg

### Debian 13+ / Ubuntu 25.04+

    sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev \
        qt6-serialport-dev qt6-tools-dev qt6-l10n-tools qt6-websockets-dev qt6-svg-dev \
        qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts \
        qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml-workerscript \
        qml6-module-qtcore qml6-module-qtquick-effects

Older distributions ship a Qt that is too old: install Qt 6.7+ with the
[Qt online installer](https://www.qt.io/download-qt-installer-oss) or
[aqtinstall](https://github.com/miurahr/aqtinstall).

### Fedora

    sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel \
        qt6-qtserialport-devel qt6-qttools-devel qt6-qtwebsockets-devel qt6-qtsvg-devel

### macOS

Install Qt 6.7+ (Qt online installer or `brew install qt`), then:

    brew install cmake ninja

### Windows

Install Visual Studio 2022 (C++ workload), CMake, Ninja and Qt 6.7+ for MSVC 2022 64-bit.
[Inno Setup 6](https://jrsoftware.org/isinfo.php) is needed only to build the installer.

## Build

From the repository root (on Windows, in a *x64 Native Tools Command Prompt*):

    cmake --preset release          # or: debug, windows-release
    cmake --build --preset release
    ./out/build/release/easymorse

If Qt is not found automatically, add `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.y/<compiler>`
to the configure command. Qt Creator and VS Code open the project directly
through `CMakeLists.txt` / `CMakePresets.json`.

Useful options:

| Option | Default | Meaning |
|--------|---------|---------|
| `EASYMORSE_BUILD_TESTS` | `ON` | Build the unit tests (run them with `ctest --preset ...` or `ctest --test-dir out/build/<preset>`) |
| `EASYMORSE_DEPLOY_QT` | `ON` on Windows/macOS | Bundle Qt libraries and plugins when installing |

## Install and packages

    cmake --install out/build/release --prefix /usr/local     # Linux
    cd out/build/release && cpack                             # create packages

`easymorse --self-test [--screenshots <dir>]` shows every page at desktop and
phone size and fails on QML warnings (it runs in `ctest`).

`cpack` produces a `.deb` and a `.tar.gz` on Linux, an Inno Setup installer and
a `.zip` on Windows, and a `.dmg` on macOS. The same packages are built by the
GitHub Actions workflow for every push.

## Practising with a correspondent

The *Network* tab connects EasyMorse directly over the Internet.

**With an EasyMorse server** (see below): enter its address, log in or create
an account and join a room. You are connected by yourself with every operator
in the room (up to 8): everyone hears everyone, as on a shared frequency, and
the decoded text shows who is sending.

**Without a server**, with invitation codes:

1. Each operator clicks *Prepare my invitation* and sends the code to the
   other (mail, chat...).
2. Each pastes the code received and clicks *Connect*.

In both cases, the *security code* of a link must be the same on both sides:
read it to each other to make sure nobody is in the middle.

The key is sent as key events, not audio: each side hears the other with its
own tone and the rhythm is restored on reception. By default the received Morse
is not decoded on screen, to learn by ear; it can be shown after each message
or live. Links go through most Internet boxes (ICE / STUN hole punching); when
they cannot go direct, the server relays them (TURN). Sessions are encrypted end
to end (X25519, XChaCha20-Poly1305): the server never sees the Morse.

## EasyMorse server

`easymorse-server` (in `server/`) hosts accounts and rooms, serves a small web
page and an administration page to moderate the accounts, and runs the
STUN/TURN relay. It is built with the application
when Qt 6.8+ HttpServer, WebSockets and Sql are installed, or alone with
`-DEASYMORSE_BUILD_APP=OFF`. See [server/deploy/README.md](server/deploy/README.md)
to run it on a VPS with Docker and HTTPS.

## Source layout

| Path | Content |
|------|---------|
| `src/core/` | `easymorse_core` library, no user interface: audio engine (`audio/`), Morse codec, sequencer and streaming decoder (`morse/`), keys (`keyer/`), `Settings` and `Trainer`, the application logic used by the interfaces |
| `src/app/` | Application: QML interface (`qml/`, desktop and phone layouts) and the models it uses |
| `src/protocol/` | Messages shared by the application and the server |
| `server/` | Match-making server, its web page and deployment files |
| `tests/` | Qt Test unit tests (`ctest`); audio tests use a silent device (`EASYMORSE_AUDIO_BACKEND=null`) |
| `3rdparty/` | Bundled libraries (miniaudio, Monocypher) and fonts (Inter, JetBrains Mono, SIL Open Font License); libjuice is used from the system or downloaded |
| `intl/`, `gfx/`, `fonts/` | Translations, icons, Morse font |

## Translations

Translation sources live in `intl/*.ts`. They are compiled and embedded into
the application at build time. After changing user-visible strings, refresh
the `.ts` files and translate them with Qt Linguist:

    cmake --build --preset release --target update_translations
    linguist intl/easymorse_fr_FR.ts
