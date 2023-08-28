cmake_minimum_required (VERSION 3.5)

set (target_name extension_manager)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/extension_manager.cpp src/extension_manager_helper.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                  ${ADU_EXTENSION_INCLUDES})

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
        ADUC_EXTENSIONS_FOLDER="${ADUC_EXTENSIONS_FOLDER}"
        ADUC_EXTENSIONS_INSTALL_FOLDER="${ADUC_EXTENSIONS_INSTALL_FOLDER}"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="${ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME}"
        ADUC_EXTENSION_REG_FILENAME="${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
        ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER="${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}"
        ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR="${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}"
)

#
# Note: add ${CMAKE_DL_LIBS} for dynamic library loading support.
#
target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types # download.h, update_content.h, and workflow.h used by header and impl
           aduc::contract_utils
    PRIVATE aduc::c_utils
            aduc::config_utils
            aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::exception_utils
            aduc::extension_utils
            aduc::hash_utils
            aduc::logging
            aduc::parser_utils
            aduc::path_utils
            aduc::string_utils
            aduc::workflow_utils
            ${CMAKE_DL_LIBS})

target_link_libraries (${target_name} PRIVATE libaducpal)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
