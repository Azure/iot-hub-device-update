set (target_name rootkeypackage_utils)

include (agentRules)
compileasc99 ()

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

add_library (${target_name} STATIC src/rootkeypackage_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aziotsharedutil
    PRIVATE Parson::parson)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
