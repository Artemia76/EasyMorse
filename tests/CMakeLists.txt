find_package(Qt6 REQUIRED COMPONENTS Test)

# easymorse_add_test(<name> SOURCES <files...> [LIBRARIES <targets...>])
function(easymorse_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/generated")
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBRARIES})
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

easymorse_add_test(tst_spscqueue SOURCES tst_spscqueue.cpp)
