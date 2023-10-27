set (target_name adps2mqtt_client_module_functional_test)

include (agentRules)
compileasc99 ()
disablertti ()

add_executable (${target_name} ${sources})
target_sources (${target_name} PRIVATE adps2mqtt_client_module_ft.cpp)

target_include_directories (
    ${target_name}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})

target_compile_definitions (${target_name}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::agent_state_store
    PRIVATE aduc::config_utils
            aduc::microsoft_adps2_mqtt_client
            aduc::parser_utils
            aduc::system_utils
            aziotsharedutil)
