cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_swupdate_2)

set (SWUPDATE_HANDLER_CONF_FILE "swupdate-handler-config.json")

set (
    ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH
    "${ADUC_CONF_FOLDER}/${SWUPDATE_HANDLER_CONF_FILE}"
    CACHE STRING "Path to the SWUpdate handler configuration file.")

# Generate result codes header from JSON definition
set (RESULT_CODES_JSON "${CMAKE_CURRENT_SOURCE_DIR}/swupdate_handler_result_codes.json")
set (RESULT_CODES_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/inc/aduc/swupdate_handler_result_codes.h")
set (RESULT_CODE_GENERATOR "${PROJECT_SOURCE_DIR}/scripts/error_code_generator_defs/extension_result_code_generator.py")

add_custom_command (
    OUTPUT ${RESULT_CODES_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/inc/aduc"
    COMMAND python3 ${RESULT_CODE_GENERATOR} -j ${RESULT_CODES_JSON} -o ${RESULT_CODES_HEADER}
    DEPENDS ${RESULT_CODES_JSON} ${RESULT_CODE_GENERATOR}
    COMMENT "Generating swupdate handler result codes header from ${RESULT_CODES_JSON}"
    VERBATIM)

# Create a custom target to ensure header is generated before compilation
add_custom_target (generate_swupdate_result_codes DEPENDS ${RESULT_CODES_HEADER})

add_library (${target_name} MODULE)

# Make the library depend on the header generation
add_dependencies (${target_name} generate_swupdate_result_codes)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/handler_create.cpp src/swupdate_handler_v2.cpp)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc ${ADUC_TYPES_INCLUDES} ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES} ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::config_utils
            aduc::contract_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::logging
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

target_link_libraries (${target_name} PRIVATE libaducpal)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}"
            ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH="${ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH}"
            ADUC_VERSION_FILE="${ADUC_VERSION_FILE}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

# Note: yocto-a-b-update.sh is now deployed by meta-raspberrypi-adu layer (Raspberry Pi specific)
# The generic ADU agent no longer includes this platform-specific script
# See: meta-raspberrypi-adu/recipes-azure-device-update/azure-device-update/azure-device-update_git.bbappend

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
