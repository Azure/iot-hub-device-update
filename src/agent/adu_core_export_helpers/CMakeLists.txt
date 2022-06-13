set (target_name adu_core_export_helpers)

include (agentRules)
compileasc99 ()

find_package (Parson REQUIRED)

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/adu_core_export_helpers.c)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types aduc::c_utils
    PRIVATE aduc::logging aduc::platform_layer Parson::parson)
