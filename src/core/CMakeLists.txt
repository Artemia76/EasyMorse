# EasyMorse core library: audio engine, Morse coding/decoding and settings.
# No widgets: shared by the application(s) and the tests.

qt_add_library(easymorse_core STATIC
    audio/AudioEngine.cpp audio/AudioEngine.h
    audio/band/bandsimulator.cpp audio/band/bandsimulator.h
    audio/band/cwstation.h
    audio/band/dsp.h
    audio/band/noise.h

    keyer/keyerinput.cpp keyer/keyerinput.h
    keyer/iambickeyer.cpp keyer/iambickeyer.h
    keyer/monotonicclock.h

    morse/keydecoder.cpp morse/keydecoder.h
    morse/morsecodec.cpp morse/morsecodec.h
    morse/contestgenerator.cpp morse/contestgenerator.h
    morse/sequencer.cpp morse/sequencer.h

    net/jitterbuffer.cpp net/jitterbuffer.h
    net/lobby.cpp net/lobby.h
    net/peerlink.cpp net/peerlink.h
    net/peersession.cpp net/peersession.h
    net/protocol.cpp net/protocol.h
    net/remotekeyer.h
    net/securechannel.cpp net/securechannel.h
    net/signalingclient.cpp net/signalingclient.h

    settings/settings.cpp settings/settings.h

    trainer.cpp trainer.h

    tools/clogger.cpp tools/clogger.h
    tools/spscqueue.h
)

target_link_libraries(easymorse_core
    PUBLIC
        easymorse_settings
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::WebSockets
        easymorse_protocol
    PRIVATE
        EasyMorse::miniaudio
        EasyMorse::monocypher
        EasyMorse::juice
)

target_include_directories(easymorse_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Serial port keys are not available on every platform (iOS)
if(TARGET Qt6::SerialPort)
    target_sources(easymorse_core PRIVATE keyer/serialkeyer.cpp keyer/serialkeyer.h)
    target_link_libraries(easymorse_core PUBLIC Qt6::SerialPort)
    target_compile_definitions(easymorse_core PUBLIC EASYMORSE_HAS_SERIALPORT)
endif()

# Metatypes let the QML application expose the core classes (QML_FOREIGN)
qt_extract_metatypes(easymorse_core)
