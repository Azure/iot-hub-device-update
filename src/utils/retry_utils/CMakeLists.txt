cmake_minimum_required (VERSION 3.5)

include (agentRules)

set (target_name retry_utils)
add_library (${target_name} STATIC src/retry_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC ./inc ${ADUC_TYPES_INCLUDES}
                                                  ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::communication_abstraction aduc::logging)

target_link_libraries (${target_name} PUBLIC libaducpal)
