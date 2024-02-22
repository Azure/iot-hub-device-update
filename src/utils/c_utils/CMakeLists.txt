cmake_minimum_required (VERSION 3.5)

set (target_name c_utils)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC src/bit_ops.c src/connection_string_utils.c
                                   src/string_c_utils.c)

add_library (aduc::${target_name} ALIAS ${target_name})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (${target_name} PRIVATE aduc::logging)

target_link_libraries (${target_name} PRIVATE libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
