cmake_minimum_required (VERSION 3.5)

set (target_name root_key_utils)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/root_key_util.c src/root_key_list.c src/root_key_store.c)

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PUBLIC)
find_package (umock_c REQUIRED CONFIG)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::crypto_utils aduc::rootkeypackage_utils umock_c
    PRIVATE aduc::logging aduc::system_utils libaducpal)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_ROOTKEY_STORE_PACKAGE_PATH="${ADUC_ROOTKEY_STORE_PACKAGE_PATH}"
                           EMBED_TEST_ROOT_KEYS=${EMBED_TEST_ROOT_KEYS})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
