set (target_name enrollment_utils)

add_library (${target_name} STATIC)

add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/adu_enrollment_utils.c)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name}
    PUBLIC aduc::mqtt_broker_common
    PRIVATE aduc::agent_state_store
            aduc::logging
            aduc::c_utils)

