cmake_minimum_required (VERSION 3.5)

project (adu_core_interface_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (
    sources
    main.cpp
    adu_core_export_helpers_ut.cpp
    adu_core_interface_ut.cpp
    adu_core_json_ut.cpp
    iothubclientmocks.cpp
    result_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES}
                                                    ${PROJECT_SOURCE_DIR}/../src
                                                    ${PROJECT_SOURCE_DIR}/../../../utils/workflow_utils/src)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_VERSION="${ADUC_VERSION}"
                            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_interface
            aduc::string_utils
            aduc::parser_utils
            aduc::workflow_utils
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

# Copy testdata to out dir
set (TEST_DATA_BUILD_DIR "${PROJECT_SOURCE_DIR}/testdata")
set (TEST_DATA_INSTALL_BASE_DIR "${ADUC_TESTDATA_OUTPUT_DIRECTORY}/agent/adu_core_interface/tests_non_plat")
file (MAKE_DIRECTORY "${Test_Data_INSTALL_BASE_DIR}")
file (COPY "${TEST_DATA_BUILD_DIR}/startupworkflowdata" DESTINATION "${TEST_DATA_INSTALL_BASE_DIR}/"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ GROUP_WRITE
                    WORLD_READ)