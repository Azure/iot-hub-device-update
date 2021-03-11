cmake_minimum_required (VERSION 3.5)

set (target_name operation_id_utils)

add_library (${target_name} STATIC src/operation_id_utils.c)
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_compile_definitions (
    ${target_name}
    PRIVATE DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH="${DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH}"
)

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils Parson::parson aziotsharedutil
    PRIVATE aduc::logging aduc::system_utils aduc::parson_json_utils diagnostics_component::diagnostics_interface)
