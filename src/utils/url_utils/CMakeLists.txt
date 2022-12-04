set (target_name url_utils)

find_package (azure_c_shared_utility REQUIRED)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/url_utils.c)
target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_libraries (${target_name} PUBLIC aziotsharedutil aduc::c_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
