
set (target_name viewstatestore)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Parson REQUIRED)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources (${target_name} PRIVATE src/viewstate_store.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE Parson::parson)

# if (ADUC_BUILD_UNIT_TESTS)
#     add_subdirectory (tests)
# endif ()
