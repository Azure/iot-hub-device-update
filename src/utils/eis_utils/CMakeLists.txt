cmake_minimum_required (VERSION 3.5)

project (eis_utils)

add_library (${PROJECT_NAME} STATIC src/eis_utils.c src/eis_coms.c src/eis_err.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils
    PRIVATE aduc::adu_types
            aduc::logging
            aduc::permission_utils
            Parson::parson
            aziotsharedutil
            uhttp)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE EIS_PIPE_PATH="${EIS_PIPE_PATH}"
    EIS_USER="${EIS_USER}"
)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)

    add_subdirectory (tests)
endif ()
