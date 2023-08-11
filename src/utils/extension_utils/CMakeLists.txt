cmake_minimum_required (VERSION 3.5)

set (target_name extension_utils)

add_library (${target_name} STATIC src/extension_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_EXTENSIONS_FOLDER="${ADUC_EXTENSIONS_FOLDER}"
        ADUC_EXTENSIONS_INSTALL_FOLDER="${ADUC_EXTENSIONS_INSTALL_FOLDER}"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="${ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME}"
        ADUC_EXTENSION_REG_FILENAME="${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
        ADUC_DOWNLOAD_HANDLER_REG_FILENAME="${ADUC_DOWNLOAD_HANDLER_REG_FILENAME}"
        ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
        ADUC_FILE_USER="${ADUC_FILE_USER}")

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (${target_name} PRIVATE libaducpal)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::config_utils
            aduc::hash_utils
            aduc::logging
            aduc::parser_utils
            aduc::path_utils
            aduc::string_utils
            aduc::system_utils
            Parson::parson)
