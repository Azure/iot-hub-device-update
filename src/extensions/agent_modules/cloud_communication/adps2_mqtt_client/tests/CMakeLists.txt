cmake_minimum_required (VERSION 3.5)

project (adps2mqtt_client_module_functional_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp adps2mqtt_client_module_ft.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_AGENT_SDK_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../mosquitto_client_extensions/inc)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${PROJECT_NAME} PRIVATE aduc::config_utils aduc::microsoft_adps2_mqtt_client;
                            aduc::parser_utils aduc::system_utils Catch2::Catch2)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
