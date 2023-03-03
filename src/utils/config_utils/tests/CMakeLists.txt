cmake_minimum_required (VERSION 3.5)

project (config_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp config_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2
            Parson::parson
            aduc::c_utils
            aduc::config_utils
            aduc::string_utils
            umock_c)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
                            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}")

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
