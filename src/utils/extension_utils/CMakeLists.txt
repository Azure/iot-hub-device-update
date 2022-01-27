cmake_minimum_required (VERSION 3.5)

project (extension_utils)

add_library (${PROJECT_NAME} STATIC src/extension_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        ADUC_EXTENSIONS_FOLDER="${ADUC_EXTENSIONS_FOLDER}"
        ADUC_EXTENSIONS_INSTALL_FOLDER="${ADUC_EXTENSIONS_INSTALL_FOLDER}"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="${ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME}"
        ADUC_EXTENSION_REG_FILENAME="${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
        ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR="${ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR}"
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR="${ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR}"
        ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR="${ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR}"
        ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
        ADUC_FILE_USER="${ADUC_FILE_USER}")

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aziotsharedutil
           aduc::adu_core_interface
           aduc::c_utils
           aduc::hash_utils
           aduc::parser_utils
           aduc::system_utils
           Parson::parson
    PRIVATE aduc::logging aduc::string_utils)
