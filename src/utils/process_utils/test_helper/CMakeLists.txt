cmake_minimum_required (VERSION 3.5)

project (process_utils_tests_helper)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp)

add_executable (${PROJECT_NAME} ${sources})

if (WIN32)
    find_package(unofficial-getopt-win32 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::getopt-win32::getopt)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::process_utils)
