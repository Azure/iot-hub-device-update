cmake_minimum_required (VERSION 3.5)

project (system_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/system_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aziotsharedutil aduc::c_utils
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
