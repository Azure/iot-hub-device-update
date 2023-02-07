cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_swupdate_1)

set (SOURCE_ALL src/swupdate_handler.cpp)

add_library (${target_name} MODULE ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

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
    PRIVATE aduc::adu_core_export_helpers
            aduc::c_utils
            aduc::contract_utils
            aduc::extension_manager
            aduc::logging
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

target_compile_definitions (${target_name} PRIVATE ADUC_VERSION_FILE="${ADUC_VERSION_FILE}"
                                                   ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
