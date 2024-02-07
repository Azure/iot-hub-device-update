cmake_minimum_required (VERSION 3.5)

set (target_name omnect_swupdate_consent_1)

set (SOURCE_ALL src/swupdate_consent_handler.cpp)


add_library (${target_name} MODULE)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/swupdate_consent_handler.cpp)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADUC_TYPES_INCLUDES}
            ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADU_EXTENSION_INCLUDES})

find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::contract_utils
            aduc::logging
            aduc::agent_workflow
            aduc::workflow_utils
            aduc::string_utils
            Parson::parson
            )

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_VERSION_FILE="${ADUC_VERSION_FILE}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
