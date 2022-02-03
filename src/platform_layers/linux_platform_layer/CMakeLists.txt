cmake_minimum_required (VERSION 3.5)

set (target_name linux_platform_layer)

include (agentRules)

compileasc99 ()
disablertti ()

add_library (${target_name} STATIC src/linux_adu_core_exports.cpp src/linux_device_info_exports.cpp
                                   src/linux_adu_core_impl.cpp)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::adu_types
            aduc::agent_workflow
            aduc::c_utils
            aduc::config_utils
            aduc::content_handlers
            aduc::exception_utils
            aduc::extension_manager
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

target_link_dosdk (${target_name} PRIVATE)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

execute_process (
    COMMAND lsb_release --id --short
    OUTPUT_VARIABLE DISTRIBUTOR_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (DISTRIBUTOR_ID STREQUAL "Debian")
    # Debian does not support manufacturer and model file, so use a hardcoded value
    target_compile_definitions (${target_name} PRIVATE ADUC_MANUFACTURER_NAME="${DISTRIBUTOR_ID}"
                                                       ADUC_MODEL_NAME="${DISTRIBUTOR_ID}")
elseif (DISTRIBUTOR_ID STREQUAL "Raspbian")
    # Raspbian does not support manufacturer and model file, so use a hardcoded value
    target_compile_definitions (${target_name} PRIVATE ADUC_MANUFACTURER_NAME="Raspberry Pi"
                                                       ADUC_MODEL_NAME="Raspbian")
endif ()
# if ADUC_MANUFACTURER_NAME or ADUC_MODEL_NAME is not specified, then the OS has the
# manufacturer and model file which we can read from.

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_DEVICEINFO_MANUFACTURER="${ADUC_DEVICEINFO_MANUFACTURER}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
            ADUC_FILE_USER="${ADUC_FILE_USER}"
            ADUC_DEVICEINFO_MODEL="${ADUC_DEVICEINFO_MODEL}"
            ADUC_VERSION_FILE="${ADUC_VERSION_FILE}"
            ADUC_BUILD_UNIT_TESTS="${ADUC_BUILD_UNIT_TESTS}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
