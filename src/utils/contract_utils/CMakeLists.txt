set (target_name contract_utils)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/contract_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

target_link_libraries (${target_name} PUBLIC aduc::c_utils)

target_link_libraries (${target_name} PRIVATE libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
