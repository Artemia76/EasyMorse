# Messages shared by EasyMorse and its match-making server (Qt Core only)

qt_add_library(easymorse_protocol STATIC
    signaling.cpp signaling.h
)
target_include_directories(easymorse_protocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(easymorse_protocol PUBLIC easymorse_settings Qt6::Core)
