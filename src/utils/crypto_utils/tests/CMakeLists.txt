cmake_minimum_required (VERSION 3.5)

project (crypto_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources crypto_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (OpenSSL REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::crypto_utils aduc::root_key_utils
                                               aduc::string_utils Catch2::Catch2WithMain OpenSSL::Crypto)

target_compile_definitions (${PROJECT_NAME} PRIVATE EMBED_TEST_ROOT_KEYS=${EMBED_TEST_ROOT_KEYS})

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
