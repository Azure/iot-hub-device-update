cmake_minimum_required (VERSION 3.5)

set (target_name c_utils)

add_library (${target_name} STATIC src/bit_ops.c src/connection_string_utils.c src/http_url.c src/string_c_utils.c )
add_library (aduc::${target_name} ALIAS ${target_name})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (azure_c_shared_utility REQUIRED)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name} PRIVATE aduc::logging aziotsharedutil)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
