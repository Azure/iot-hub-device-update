cmake_minimum_required (VERSION 3.5)

project (crypto_utils)

add_library (${PROJECT_NAME} STATIC src/crypto_lib.c src/base64_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

find_package (azure_c_shared_utility REQUIRED)
find_package (OpenSSL REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils aziotsharedutil
    # PRIVATE aduc::root_key_utils OpenSSL::Crypto)
    PRIVATE OpenSSL::Crypto)

target_compile_definitions (${PROJECT_NAME} PRIVATE EMBED_TEST_ROOT_KEYS=${EMBED_TEST_ROOT_KEYS})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
