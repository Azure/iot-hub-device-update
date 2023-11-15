set (target_name enrollment_utils)

add_library (${target_name} STATIC src/adu_enrollment_utils.c)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name}
    PUBLIC aduc::mqtt_broker_common
    PRIVATE aduc::agent_state_store
            aduc::logging
            aduc::parson_json_utils
            aduc::c_utils)

add_subdirectory (tests)
