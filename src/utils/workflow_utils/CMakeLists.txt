cmake_minimum_required (VERSION 3.5)

set (target_name workflow_utils)
include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/workflow_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE src)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_compile_definitions (
    ${target_name}
    PRIVATE SUPPORTED_UPDATE_MANIFEST_VERSION_MIN=${SUPPORTED_UPDATE_MANIFEST_VERSION_MIN}
            SUPPORTED_UPDATE_MANIFEST_VERSION_MAX=${SUPPORTED_UPDATE_MANIFEST_VERSION_MAX})

target_link_aziotsharedutil (${target_name} PUBLIC)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::c_utils
            aduc::config_utils
            aduc::extension_manager
            aduc::hash_utils
            aduc::jws_utils
            aduc::logging
            aduc::parser_utils
            aduc::path_utils
            aduc::reporting_utils
            aduc::root_key_utils
            aduc::system_utils
            libaducpal
            Parson::parson)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
