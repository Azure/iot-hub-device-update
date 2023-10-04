cmake_minimum_required (VERSION 3.5)

set (target_name crypto_utils)

add_library (${target_name} STATIC src/crypto_lib.c src/root_key_util.c src/base64_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (OpenSSL REQUIRED)

message(OPENSSL_CRYPTO_LIBRARY="${OPENSSL_CRYPTO_LIBRARY}")
message(OPENSSL_VERSION="${OPENSSL_VERSION}")
message(OPENSSL_INCLUDE_DIR="${OPENSSL_INCLUDE_DIR}")
message(OPENSSL_SSL_LIBRARY="${OPENSSL_SSL_LIBRARY}")
#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE OpenSSL::Crypto libaducpal)


# Always support test root keys.
add_definitions (-DBUILD_WITH_TEST_KEYS=1)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
