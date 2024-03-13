cmake_minimum_required (VERSION 3.5)

set (target_name config_utils)
include (agentRules)

compileasc99 ()
add_library (${target_name} STATIC src/config_utils.c src/config_parsefile.c)
add_library (aduc::${target_name} ALIAS ${target_name})

set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_include_directories (${target_name} PUBLIC inc)

target_compile_definitions (${target_name} PRIVATE ADUSHELL_FOLDER="${ADUSHELL_FOLDER}"
                                                    ADUSHELL_FILENAME="${ADUSHELL_FILENAME}"
                                                    ADUC_CONF_FILE="${ADUC_CONF_FILE}"
                                                    ADUC_CONF_FOLDER="${ADUC_CONF_FOLDER}"
                                                    ADUC_DATA_FOLDER="${ADUC_DATA_FOLDER}"
                                                    ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR="${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}"
                                                    ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER="${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}"
                                                    ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
                                                    ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS}"
                                                    ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}")

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
           Parson::parson
    PRIVATE
            aduc::logging
            aduc::parson_json_utils)

target_link_libraries (${target_name} PUBLIC libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
