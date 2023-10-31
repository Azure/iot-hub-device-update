cmake_minimum_required (VERSION 3.5)

set (target_name adu_mqtt_client_module_functional_test)

include (agentRules)

disablertti ()

find_package (azure_c_shared_utility REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE adu_mqtt_client_module_ft.cpp)

target_include_directories (
    ${target_name}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_AGENT_SDK_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../mosquitto_client_extensions/inc)

target_compile_definitions (${target_name}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${target_name}
    PUBLIC aduc::agent_state_store
            aziotsharedutil
    PRIVATE aduc::config_utils
            aduc::microsoft_du_mqttbroker_client;
            aduc::microsoft_adps2_mqtt_client;
            aduc::parser_utils
            aduc::system_utils
            )
