project (contract_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/contract_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

target_link_libraries (${PROJECT_NAME} PUBLIC aduc::c_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
