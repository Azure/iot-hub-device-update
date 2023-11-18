set (target_name reporting_utils)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC src/reporting_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_link_aziotsharedutil (${target_name} PUBLIC)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
