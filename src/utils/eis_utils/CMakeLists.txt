cmake_minimum_required (VERSION 3.5)

set (target_name eis_utils)
add_library (${target_name} STATIC src/eis_utils.c src/eis_coms.c src/eis_err.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::adu_types
            aduc::logging
            aduc::permission_utils
            Parson::parson
            uhttp)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_BUILD_SNAP="${ADUC_BUILD_SNAP}"
            EIS_DATA_PATH="${EIS_DATA_PATH}"
            EIS_USER="${EIS_USER}"
)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
