cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_interface)

include (agentRules)

compileasc99 ()

# Used to find and include the CURL::libcurl imported libary
find_curl_and_import_libcurl ()

add_library (${target_name} STATIC src/diagnostics_interface.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_compile_definitions (${target_name}
                            PRIVATE DIAGNOSTICS_CONFIG_FILE_PATH="${DIAGNOSTICS_CONFIG_FILE_PATH}")

target_link_digital_twin_client (${target_name} PUBLIC)

find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
           aduc::communication_abstraction
           diagnostics_component::diagnostics_workflow
           Parson::parson
    PRIVATE aduc::logging aduc::pnp_helper IotHubClient::iothub_client)
