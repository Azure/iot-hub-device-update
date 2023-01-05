cmake_minimum_required (VERSION 3.5)

set (target_name file_info_utils)

add_library (${target_name} STATIC src/file_info_utils.c)
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

target_link_libraries (${target_name} PRIVATE libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    # target_link_umock_c (${target_name} PRIVATE)
    add_subdirectory (tests)
endif ()
