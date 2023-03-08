cmake_minimum_required (VERSION 3.5)

project (crypto_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp crypto_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::crypto_utils aduc::string_utils Catch2::Catch2)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}" USE_LOCAL_STORE=1)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
