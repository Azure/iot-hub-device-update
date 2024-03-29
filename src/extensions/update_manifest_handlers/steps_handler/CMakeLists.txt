cmake_minimum_required (VERSION 3.5)

#
# Specify the update handler name.
# This should match the associated update type.
# e.g. 'microsoft/steps:1' ==> 'microsoft_steps_1'
#
set (target_name microsoft_steps_1)

find_package (Parson REQUIRED)
find_package (IotHubClient REQUIRED)

add_library (${target_name} MODULE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/steps_handler.cpp src/handler_create.cpp)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc ${ADUC_TYPES_INCLUDES} ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES} ${ADU_EXTENSION_INCLUDES})

get_filename_component (ADUC_INSTALLEDCRITERIA_FILE_PATH
                        "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}" ABSOLUTE "/")

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

# Windows needs all DLLS to have links to all potential libraries so this is included here
if (WIN32)

    target_link_dosdk (${target_name} PRIVATE)

endif ()

target_link_libraries (
    ${target_name}
    PRIVATE aduc::agent_workflow
            aduc::contract_utils
            aduc::c_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::extension_utils
            aduc::logging
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (${target_name} PRIVATE libaducpal)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
