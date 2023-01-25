cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_simulator_1)

add_library (${target_name} MODULE)
target_sources (${target_name} PRIVATE src/simulator_handler.cpp)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADUC_EXPORT_INCLUDES}
            ${ADUC_TYPES_INCLUDES})

get_filename_component (
    ADUC_INSTALLEDCRITERIA_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}"
    ABSOLUTE
    "/")

find_package (azure_c_shared_utility REQUIRED)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_link_libraries (
    ${target_name}
    PRIVATE aduc::agent_workflow
            aduc::contract_utils
            aduc::logging
            aduc::parser_utils
            aduc::workflow_utils
            Parson::parson)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
