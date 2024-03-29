cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_script_1)

add_library (${target_name} MODULE "")

add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/script_handler.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES} ${ADU_SHELL_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::adu_core_export_helpers
            aduc::c_utils
            aduc::config_utils
            aduc::contract_utils
            aduc::exception_utils
            aduc::extension_utils
            aduc::extension_manager
            aduc::logging
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

target_link_libraries (${target_name} PUBLIC libaducpal)

# Windows needs all DLLS to have links to all potential libraries so this is included here
if (WIN32)

    target_link_dosdk (${target_name} PRIVATE)

endif ()

target_compile_definitions (${target_name} PRIVATE ADUSHELL_FILE_PATH="${ADUSHELL_FILE_PATH}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

if (ADUC_BUILD_UNIT_TESTS)

    add_subdirectory (tests)

endif ()
