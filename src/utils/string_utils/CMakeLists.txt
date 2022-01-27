cmake_minimum_required (VERSION 3.5)

set (target_name string_utils)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc)

find_package (azure_c_shared_utility REQUIRED)
target_link_libraries (${target_name} INTERFACE aziotsharedutil)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
