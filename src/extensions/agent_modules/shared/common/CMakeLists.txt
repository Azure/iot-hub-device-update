
set (target_name mqtt_broker_common)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

# Find the Mosquitto library package
find_package (PkgConfig REQUIRED)
pkg_search_module (MOSQUITTO REQUIRED libmosquitto)

target_sources (${target_name} PRIVATE src/adu_mqtt_common.c src/adu_mqtt_protocol.c src/adu_upd.c)

target_include_directories (${target_name}
    PUBLIC
        inc
        ${MOSQUITTO_INCLUDE_DIRS})

target_link_libraries(${target_name}
    PUBLIC
        aduc::c_utils
        aduc::communication_channel_mqtt_broker
        aduc::agent_state_store
        aduc::logging
        aduc::retry_utils
        ${MOSQUITTO_LIBRARIES})

target_link_libraries (${target_name}
    PRIVATE
        aduc::du_agent_sdk)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
