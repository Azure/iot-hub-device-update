cmake_minimum_required (VERSION 3.5)

project (file_info_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp file_info_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (OpenSSL REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::jws_utils
            aduc::crypto_utils
            aduc::string_utils
            diagnostic_utils::file_info_utils
            Catch2::Catch2
            OpenSSL::Crypto
            aziotsharedutil)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
