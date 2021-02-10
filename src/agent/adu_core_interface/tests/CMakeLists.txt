cmake_minimum_required (VERSION 3.5)

project (adu_core_interface_unit_tests)

if (NOT
    ${ADUC_PLATFORM_LAYER}
    STREQUAL
    "simulator")
    message (FATAL_ERROR "${PROJECT_NAME} requires the simulator platform layer.")
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (
    sources
    main.cpp
    adu_core_json_ut.cpp
    adu_core_interface_ut.cpp
    adu_core_export_helpers_ut.cpp
    result_ut.cpp
    agent_workflow_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)

add_executable (${PROJECT_NAME} ${sources})

# "${PROJECT_SOURCE_DIR}/../src" for agent_workflow_utils.h

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES}
                                                    ${PROJECT_SOURCE_DIR}/../src)
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_VERSION="${ADUC_VERSION}"
            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_interface
            aduc::string_utils
            Catch2::Catch2
            umock_c)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})

if (ENABLE_ADU_TELEMETRY_REPORTING)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ENABLE_ADU_TELEMETRY_REPORTING)
endif ()
