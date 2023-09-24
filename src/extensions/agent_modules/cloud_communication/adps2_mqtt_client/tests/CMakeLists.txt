cmake_minimum_required (VERSION 3.5)

project (adps2mqtt_client_module_functional_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources adps2mqtt_client_module_ft.cpp)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_AGENT_SDK_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../inc)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::agent_state_store
    PRIVATE aduc::config_utils
            aduc::microsoft_adps2_mqtt_client
            aduc::parser_utils
            aduc::system_utils)
