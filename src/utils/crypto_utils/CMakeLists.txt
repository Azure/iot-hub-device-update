cmake_minimum_required (VERSION 3.5)

project (crypto_utils)

add_library (${PROJECT_NAME} STATIC src/crypto_lib.c src/root_key_util.c src/base64_utils.c)
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
    PUBLIC aduc::c_utils
    PRIVATE aziotsharedutil OpenSSL::Crypto)

# Always support test root keys.
add_definitions (-DBUILD_WITH_TEST_KEYS=1)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # Windows uses OpenSSL, but there are deprecated APIs there. Allow them.
    # TODO(JeffMill): Use target_compile_definitions instead?
    # https://www.openssl.org/docs/man3.0/man7/OPENSSL_NO_DEPRECATED.html
    add_compile_definitions (OPENSSL_API_COMPAT=10002)
endif ()

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
