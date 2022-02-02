cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_swupdate_2)

set (SWUPDATE_HANDLER_CONF_FILE "swupdate-handler-config.json")

set (
    ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH
    "${ADUC_CONF_FOLDER}/${SWUPDATE_HANDLER_CONF_FILE}"
    CACHE STRING "Path to the SWUpdate handler configuration file.")

add_library (${target_name} SHARED "")

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/handler_create.cpp src/swupdate_handler_v2.cpp)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADUC_TYPES_INCLUDES}
            ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::contract_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_VERSION_FILE="${ADUC_VERSION_FILE}" ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}"
            ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH="${ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

if (ADUC_BUILD_UNIT_TESTS)

    add_subdirectory (tests)

endif ()
