set (target_name url_utils)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_link_aziotsharedutil (${target_name} PUBLIC)

target_sources (${target_name} PRIVATE src/https_proxy_utils.c src/url_utils.c)
target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
