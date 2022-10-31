set (target_name diagnostics_config_utils)

add_library (${target_name} STATIC)
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)
target_sources (${target_name} PRIVATE src/diagnostics_config_utils.c)

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aziotsharedutil Parson::parson
    PRIVATE aduc::parson_json_utils aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
