cmake_minimum_required (VERSION 3.5)

set (target_name file_info_utils)

add_library (${target_name} STATIC src/file_info_utils.c)
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aziotsharedutil aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${target_name} PRIVATE umock_c)

    add_subdirectory(tests)
endif ()
