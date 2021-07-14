cmake_minimum_required (VERSION 3.5)

project (pvcontrol_handler_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp pvcontrol_simulator_handler_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES} ${PROJECT_SOURCE_DIR}/../inc
                            ${PROJECT_SOURCE_DIR}/../../inc)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_interface
            aduc::exception_utils
            Catch2::Catch2
            umock_c)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
