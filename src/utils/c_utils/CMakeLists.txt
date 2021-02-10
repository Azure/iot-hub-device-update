cmake_minimum_required (VERSION 3.5)

set (target_name c_utils)

add_library (${target_name} STATIC src/string_c_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

find_package (azure_c_shared_utility REQUIRED)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name} PRIVATE aziotsharedutil)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
