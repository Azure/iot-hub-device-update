cmake_minimum_required (VERSION 3.5)

set (target_name system_utils)
include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/system_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
find_package (azure_c_shared_utility REQUIRED)
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aziotsharedutil
    PRIVATE aduc::logging)

target_compile_definitions (${target_name} PRIVATE ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
                                                   ADUC_FILE_USER="${ADUC_FILE_USER}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
