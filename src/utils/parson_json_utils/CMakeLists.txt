cmake_minimum_required (VERSION 3.5)

set (target_name parson_json_utils)
add_library (${target_name} STATIC src/parson_json_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})
target_include_directories (${target_name} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils Parson::parson
    PRIVATE aduc::logging)

target_link_aziotsharedutil (${target_name} PRIVATE)
target_link_umock_c (${target_name} PRIVATE)
