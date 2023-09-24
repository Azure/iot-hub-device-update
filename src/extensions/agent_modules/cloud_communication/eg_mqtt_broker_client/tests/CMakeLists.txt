cmake_minimum_required (VERSION 3.5)

project (adu_mqtt_client_module_functional_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources adu_mqtt_client_module_ft.cpp)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_AGENT_SDK_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../mosquitto_client_extensions/inc)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::agent_state_store
    PRIVATE aduc::config_utils
            aduc::microsoft_du_mqttbroker_client;
            aduc::parser_utils
            aduc::system_utils)
