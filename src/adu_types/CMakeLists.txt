cmake_minimum_required (VERSION 3.5)

set (target_name adu_types)

add_library (${target_name} STATIC src/adu_types.c)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${target_name} PUBLIC aduc::c_utils aduc::logging Parson::parson)
