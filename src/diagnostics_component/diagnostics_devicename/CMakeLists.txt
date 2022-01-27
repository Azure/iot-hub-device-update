cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_devicename)

add_library (${target_name} STATIC src/diagnostics_devicename.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

# NOTE: the call to find_package for azure_c_shared_utility
find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aziotsharedutil)
