# easymorse-server: accounts, rooms, signaling and STUN/TURN relay

include(GNUInstallDirs)

# Server code without main(), shared with the tests
qt_add_library(easymorse_server_lib STATIC
    accounts.cpp accounts.h
    server.cpp server.h
    turnrelay.cpp turnrelay.h
)
target_include_directories(easymorse_server_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(easymorse_server_lib
    PUBLIC
        easymorse_protocol
        Qt6::Core
        Qt6::Network
        Qt6::HttpServer
        Qt6::WebSockets
        Qt6::Sql
    PRIVATE
        EasyMorse::monocypher
        EasyMorse::juice
)
qt_add_resources(easymorse_server_lib server_web
    PREFIX /
    FILES
        web/index.html
        web/app.js
        web/style.css
)

qt_add_executable(easymorse-server main.cpp)
target_link_libraries(easymorse-server PRIVATE easymorse_server_lib)
target_compile_definitions(easymorse-server PRIVATE EASYMORSE_SERVER_VERSION="${PROJECT_VERSION}")
set_target_properties(easymorse-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
install(TARGETS easymorse-server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT server EXCLUDE_FROM_ALL)
