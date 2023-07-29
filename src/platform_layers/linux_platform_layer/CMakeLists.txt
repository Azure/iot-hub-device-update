cmake_minimum_required (VERSION 3.5)

set (target_name linux_platform_layer)

include (agentRules)

compileasc99 ()
disablertti ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (
    ${target_name} PRIVATE src/linux_adu_core_exports.cpp src/linux_device_info_exports.cpp
                           src/linux_adu_core_impl.cpp)

target_include_directories (${target_name} PUBLIC ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC atomic
    PRIVATE aduc::adu_types
            aduc::agent_workflow
            aduc::c_utils
            aduc::contract_utils
            aduc::config_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            aduc::shutdown_service
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_DEVICEINFO_MANUFACTURER="${ADUC_DEVICEINFO_MANUFACTURER}"
            ADUC_DEVICEINFO_MODEL="${ADUC_DEVICEINFO_MODEL}"
            ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
            ADUC_FILE_USER="${ADUC_FILE_USER}"
            ADUC_VERSION_FILE="${ADUC_VERSION_FILE}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
