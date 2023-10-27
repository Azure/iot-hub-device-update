
set (target_name mqtt_broker_common)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

# Find the Mosquitto library package
find_package (PkgConfig REQUIRED)
pkg_search_module (MOSQUITTO REQUIRED libmosquitto)

target_include_directories (${target_name}
    INTERFACE
        inc
        ${MOSQUITTO_INCLUDE_DIRS})

target_link_libraries(${target_name}
    INTERFACE
        aduc::retry_utils
        ${MOSQUITTO_LIBRARIES})
