cmake_minimum_required (VERSION 3.5)

set (target_name mosquitto_utils)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Find the Mosquitto library package
find_package (PkgConfig REQUIRED)
pkg_search_module (MOSQUITTO REQUIRED libmosquitto)

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC ./inc ${MOSQUITTO_INCLUDE_DIRS})

target_sources (${target_name} PRIVATE src/adu_mosquitto_utils.c)

target_link_libraries (${target_name} PRIVATE aduc::c_utils libaducpal ${MOSQUITTO_LIBRARIES})
