cmake_minimum_required (VERSION 3.5)

project (process_utils_tests_helper)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::process_utils)
