cmake_minimum_required (VERSION 3.5)

project (process_utils_tests_helper)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp)

add_executable (${PROJECT_NAME} ${sources})

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # TODO(JeffMill): [PAL] getopt.h
    find_path (GETOPT_INCLUDE_DIR "getopt.h")
    target_include_directories (${PROJECT_NAME} PRIVATE ${GETOPT_INCLUDE_DIR})

    find_library (GETOPT_LIBRARY "getopt.lib")
    target_link_libraries (${PROJECT_NAME} PRIVATE ${GETOPT_LIBRARY})
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::process_utils)
