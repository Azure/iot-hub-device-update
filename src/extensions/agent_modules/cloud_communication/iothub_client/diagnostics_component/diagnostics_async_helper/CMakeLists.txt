cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_async_helper)

add_library (${target_name} STATIC src/diagnostics_async_helper.cpp)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging
            aduc::string_utils
            diagnostics_component::diagnostics_workflow
            diagnostic_utils::operation_id_utils)
