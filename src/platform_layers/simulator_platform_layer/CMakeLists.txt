cmake_minimum_required (VERSION 3.5)

set (target_name simulator_platform_layer)

include (agentRules)

compileasc99 ()
disablertti ()

add_library (
    ${target_name} STATIC
    src/simulator_adu_core_exports.cpp
    src/simulator_adu_core_impl.cpp
    src/simulator_device_info.cpp
    src/simulator_device_info_exports.cpp
    src/uhttp_downloader.cpp)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC ${ADUC_EXPORT_INCLUDES})

find_package (azure_c_shared_utility REQUIRED)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_DEVICEINFO_MANUFACTURER="${ADUC_DEVICEINFO_MANUFACTURER}"
                           ADUC_DEVICEINFO_MODEL="${ADUC_DEVICEINFO_MODEL}")

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::string_utils
            aduc::logging
            aduc::exception_utils
            aduc::content_handlers
            aziotsharedutil
            uhttp)
