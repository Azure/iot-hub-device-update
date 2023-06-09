cmake_minimum_required (VERSION 3.5)

set (target_name root_key_utils)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/root_key_util.c src/root_key_list.c)

target_include_directories (${target_name} PUBLIC inc)

find_package (azure_c_shared_utility REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::crypto_utils aduc::rootkeypackage_utils
    PRIVATE aduc::logging aduc::system_utils aziotsharedutil)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_ROOTKEY_STORE_PACKAGE_PATH="${ADUC_ROOTKEY_STORE_PACKAGE_PATH}"
                           EMBED_TEST_ROOT_KEYS=${EMBED_TEST_ROOT_KEYS})

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${target_name} PRIVATE umock_c)

    add_subdirectory (tests)
endif ()
