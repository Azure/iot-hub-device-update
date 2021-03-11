cmake_minimum_required (VERSION 3.5)

project (adu_core_interface_unit_tests)

include (agentRules)
include (aduc_helpers)

compileasc99 ()
disablertti ()

set (
    sources
    main.cpp
    adu_core_export_helpers_ut.cpp
    adu_core_interface_ut.cpp
    adu_core_json_ut.cpp
    result_ut.cpp
    startup_workflowdata_ut.cpp
    workflow_reboot_ut.cpp
    workflow_replacement_ut.cpp
    workflow_test_utils.cpp
    workflow_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)
find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED) 

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_VERSION="${ADUC_VERSION}"
                            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}"
                            ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}"
                            ADUC_BUILD_UNIT_TESTS="${ADUC_BUILD_UNIT_TESTS}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_interface
            aduc::c_utils aduc::adu_types
            aduc::content_handlers
            aduc::string_utils
            aduc::parser_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Catch2::Catch2
            aziotsharedutil
            IotHubClient::iothub_client
            iothub_client_mqtt_transport
            umqtt
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
