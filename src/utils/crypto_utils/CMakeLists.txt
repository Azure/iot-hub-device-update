cmake_minimum_required (VERSION 3.5)

set (target_name crypto_utils)

add_library (${target_name} STATIC src/crypto_lib.c src/base64_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (OpenSSL REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_aziotsharedutil (${target_name} PUBLIC)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::root_key_utils OpenSSL::Crypto)

target_link_libraries (${target_name} PRIVATE libaducpal)

target_compile_definitions (${target_name} PRIVATE EMBED_TEST_ROOT_KEYS=${EMBED_TEST_ROOT_KEYS})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
