find_package(Qt6 REQUIRED COMPONENTS Test)

# easymorse_add_test(<name> SOURCES <files...> [LIBRARIES <targets...>])
function(easymorse_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/src/core")
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBRARIES})
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

if(TARGET easymorse_core)
    easymorse_add_test(tst_spscqueue SOURCES tst_spscqueue.cpp)
    easymorse_add_test(tst_morsecodec SOURCES tst_morsecodec.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_sequencer SOURCES tst_sequencer.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_playback SOURCES tst_playback.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_radiochannel SOURCES tst_radiochannel.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_bandsimulator SOURCES tst_bandsimulator.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_keydecoder SOURCES tst_keydecoder.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_keyer SOURCES tst_keyer.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_settings SOURCES tst_settings.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_trainer SOURCES tst_trainer.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_protocol SOURCES tst_protocol.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_jitterbuffer SOURCES tst_jitterbuffer.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_peerlink SOURCES tst_peerlink.cpp LIBRARIES easymorse_core)
    easymorse_add_test(tst_peersession SOURCES tst_peersession.cpp LIBRARIES easymorse_core)
endif()

if(TARGET easymorse_server_lib)
    easymorse_add_test(tst_server SOURCES tst_server.cpp LIBRARIES easymorse_server_lib)
endif()

# Application and server together
if(TARGET easymorse_core AND TARGET easymorse_server_lib)
    easymorse_add_test(tst_lobby SOURCES tst_lobby.cpp LIBRARIES easymorse_server_lib easymorse_core)
endif()

if(TARGET easymorse)
    # The QML interface loads every page at desktop and phone size without
    # QML warnings (silent audio, settings in the build tree)
    add_test(NAME qml_self_test COMMAND easymorse --self-test)
    set_tests_properties(qml_self_test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;EASYMORSE_AUDIO_BACKEND=null;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/qml-self-test-config"
        TIMEOUT 120
    )
endif()
