cmake_minimum_required (VERSION 3.5)

project (extension_manager)

add_library (${PROJECT_NAME} STATIC)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE src/extension_manager.cpp src/extension_manager_helper.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                   ${ADU_EXTENSION_INCLUDES})

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        ADUC_EXTENSIONS_FOLDER="${ADUC_EXTENSIONS_FOLDER}"
        ADUC_EXTENSIONS_INSTALL_FOLDER="${ADUC_EXTENSIONS_INSTALL_FOLDER}"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="${ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME}"
        ADUC_EXTENSION_REG_FILENAME="${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
        ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER="${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}"
        ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR="${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}"
        ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR="${ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR}"
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR="${ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR}"
        ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR="${ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR}")

#
# Note: add ${CMAKE_DL_LIBS} for dynamic library loading support.
#
target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types # download.h, update_content.h, and workflow.h used by header and impl
    PRIVATE aduc::c_utils
            aduc::contract_utils
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

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
