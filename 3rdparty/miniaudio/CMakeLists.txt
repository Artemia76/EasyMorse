# miniaudio 0.11.26 - https://github.com/mackron/miniaudio
# Public Domain / MIT-0 (see LICENSE). Only the low level device API is used.

add_library(miniaudio STATIC miniaudio.c miniaudio.h)
add_library(EasyMorse::miniaudio ALIAS miniaudio)

target_include_directories(miniaudio SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(miniaudio PUBLIC
    MA_NO_DECODING
    MA_NO_ENCODING
    MA_NO_GENERATION
    MA_NO_RESOURCE_MANAGER
    MA_NO_NODE_GRAPH
    MA_NO_ENGINE
)
set_target_properties(miniaudio PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(IOS)
    # No dlopen() of the audio frameworks on iOS: link them
    target_compile_definitions(miniaudio PUBLIC MA_NO_RUNTIME_LINKING)
    # Its audio session code is written in Objective-C
    target_compile_options(miniaudio PRIVATE -x objective-c)
    target_link_libraries(miniaudio PUBLIC
        "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox" "-framework AVFoundation")
endif()

find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
    target_link_libraries(miniaudio PUBLIC m)
endif()
