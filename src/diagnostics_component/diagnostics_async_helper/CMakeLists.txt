cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_async_helper)

add_library (${target_name} STATIC src/diagnostics_async_helper.cpp)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aziotsharedutil
            aduc::logging
            aduc::string_utils
            diagnostics_component::diagnostics_workflow
            diagnostic_utils::operation_id_utils)
