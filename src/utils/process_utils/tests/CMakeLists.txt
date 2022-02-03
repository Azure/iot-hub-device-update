cmake_minimum_required (VERSION 3.5)

project (process_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp process_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::process_utils aziotsharedutil Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
