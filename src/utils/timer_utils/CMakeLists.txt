
set (target_name timer_utils)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Parson REQUIRED)

add_library (${target_name} OBJECT)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/timer.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
