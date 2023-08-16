cmake_minimum_required (VERSION 3.5)

set (target_name workflow_data_utils)
include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/workflow_data_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::adu_core_export_helpers
            aduc::adu_types
            aduc::c_utils
            aduc::workflow_utils)

target_link_aziotsharedutil (${target_name} PRIVATE)
