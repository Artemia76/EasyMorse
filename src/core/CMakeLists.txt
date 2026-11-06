# EasyMorse core library: audio engine, Morse coding/decoding and settings.
# No widgets: shared by the application(s) and the tests.

qt_add_library(easymorse_core STATIC
    audio/AudioEngine.cpp audio/AudioEngine.h
    audio/DefaultParameters.h
    audio/IOscillatorFunction.h
    audio/Saw.cpp
    audio/Sine.cpp
    audio/Square.cpp
    audio/Triangle.cpp
    audio/envelope/Envelope.cpp audio/envelope/Envelope.h
    audio/envelope/IEnvelope.h
    audio/envelope/NoEnvelope.h
    audio/filter/BandpassFilter.cpp audio/filter/BandpassFilter.h
    audio/filter/FilterTypes.h
    audio/filter/HighpassFilter.cpp audio/filter/HighpassFilter.h
    audio/filter/IFilter.h
    audio/filter/LowpassFilter.cpp audio/filter/LowpassFilter.h
    audio/filter/NoFilter.h
    audio/modulation/ModulationValue.cpp audio/modulation/ModulationValue.h
    audio/oscillator/ILfo.h
    audio/oscillator/Lfo.cpp audio/oscillator/Lfo.h
    audio/oscillator/NoOscillator.h
    audio/voice/INoise.h
    audio/voice/IVoice.h
    audio/voice/Noise.cpp audio/voice/Noise.h
    audio/voice/Voice.cpp audio/voice/Voice.h
    audio/voicemanager/Param.cpp audio/voicemanager/Param.h
    audio/voicemanager/VoiceManager.cpp audio/voicemanager/VoiceManager.h

    keyer/keyerinput.cpp keyer/keyerinput.h
    keyer/monotonicclock.h

    morse/keydecoder.cpp morse/keydecoder.h
    morse/morsecodec.cpp morse/morsecodec.h
    morse/sequencer.cpp morse/sequencer.h

    net/jitterbuffer.cpp net/jitterbuffer.h
    net/peerlink.cpp net/peerlink.h
    net/peersession.cpp net/peersession.h
    net/protocol.cpp net/protocol.h
    net/remotekeyer.h
    net/securechannel.cpp net/securechannel.h

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
