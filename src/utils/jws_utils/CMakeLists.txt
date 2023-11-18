cmake_minimum_required (VERSION 3.5)

set (target_name jws_utils)

include (agentRules)

compileasc99 ()
add_library (${target_name} STATIC src/jws_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::crypto_utils aduc::c_utils
    PRIVATE Parson::parson aduc::root_key_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
