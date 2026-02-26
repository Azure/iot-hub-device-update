cmake_minimum_required (VERSION 3.5)

project (health_management_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} health_management_ut.cpp
                                ${CMAKE_SOURCE_DIR}/src/agent/src/health_management.c)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/inc
            ${CMAKE_SOURCE_DIR}/src/utils/config_utils/inc
            ${CMAKE_SOURCE_DIR}/src/utils/permission_utils/inc
            ${CMAKE_SOURCE_DIR}/src/utils/system_utils/inc
            ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Parson::parson
            aduc::c_utils
            aduc::adu_types
            aduc::string_utils)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_FILE_USER="adu"
            ADUC_FILE_GROUP="adu"
            DO_FILE_USER="do"
            DO_FILE_GROUP="do"
            ADUC_CONF_FOLDER="/etc/adu"
            ADUC_CONF_FILE_PATH="/etc/adu/du-config.json"
            ADUC_LOG_FOLDER="/var/log/adu"
            ADUC_DATA_FOLDER="/var/lib/adu"
            ADUC_AGENT_FILEPATH="/usr/bin/AducIotAgent"
            ADUC_DOWNLOADS_FOLDER="/var/lib/adu/downloads"
            ADUC_EXTENSIONS_FOLDER="/var/lib/adu/extensions")

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
