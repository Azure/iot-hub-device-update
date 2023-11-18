cmake_minimum_required (VERSION 3.5)

project (jws_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp jws_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (OpenSSL REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::jws_utils
            aduc::crypto_utils
            aduc::root_key_utils
            aduc::rootkeypackage_utils
            aduc::string_utils
            aduc::system_utils
            Catch2::Catch2
            OpenSSL::Crypto)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
