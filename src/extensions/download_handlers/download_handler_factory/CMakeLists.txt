set (target_name download_handler_factory)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/download_handler_factory.cpp)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::adu_types
            aduc::c_utils
            aduc::download_handler_plugin
            aduc::extension_utils
            aduc::hash_utils
            aduc::logging
            aduc::parser_utils)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_DOWNLOAD_HANDLER_EXTENSION_DIR="${ADUC_DOWNLOAD_HANDLER_EXTENSION_DIR}"
            ADUC_DOWNLOAD_HANDLER_REG_FILENAME="${ADUC_DOWNLOAD_HANDLER_REG_FILENAME}")
