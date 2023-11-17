cmake_minimum_required (VERSION 3.5)

project (root_key_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)

set (sources main.cpp root_key_utils_ut.cpp)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE main.cpp root_key_utils_ut.cpp)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::root_key_utils aduc::rootkeypackage_utils
                                               aduc::string_utils Catch2::Catch2 umock_c)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
