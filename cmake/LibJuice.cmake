# Locate libjuice (ICE / STUN / TURN, MPL-2.0) and expose it as EasyMorse::juice.
# Uses an installed libjuice when available, otherwise downloads v1.7.4 and
# builds it statically.

find_package(LibJuice CONFIG QUIET)
if(TARGET LibJuice::LibJuice)
    add_library(EasyMorse::juice ALIAS LibJuice::LibJuice)
else()
    message(STATUS "libjuice not found on the system, building it from source")
    include(FetchContent)
    set(NO_TESTS ON CACHE BOOL "" FORCE)
    set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
    FetchContent_Declare(libjuice
        GIT_REPOSITORY https://github.com/paullouisageneau/libjuice.git
        GIT_TAG b89c792e3612faf2f12cf35bcc56857313a06be3 # v1.7.4
        SYSTEM
        # Only the static library is wanted, not its install rules
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(libjuice)
    set_target_properties(juice-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(EasyMorse::juice ALIAS juice-static)
endif()
