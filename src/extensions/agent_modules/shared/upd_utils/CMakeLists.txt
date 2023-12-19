set (target_name upd_utils)

add_library (${target_name} STATIC src/adu_upd_utils.c)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (Parson REQUIRED)
target_link_libraries (${target_name}
    PUBLIC aduc::retry_utils Parson::parson
    PRIVATE aduc::agent_state_store
            aduc::c_utils
            aduc::logging
            aduc::mqtt_broker_common
            aduc::workqueue)
