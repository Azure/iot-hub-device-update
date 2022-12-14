cmake_minimum_required (VERSION 3.5)

project (eis_utils)

add_library (${PROJECT_NAME} STATIC src/eis_utils.c src/eis_coms.c src/eis_err.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

if (_WIN32)
    # TODO(JeffMill): Avoid cannot open file 'uhttp.lib'
    find_library (UHTTP_LIBRARY uhttp REQUIRED)
    cmake_path (
        GET
        UHTTP_LIBRARY
        PARENT_PATH
        UHTTP_LIBRARY_DIR)
    message (STATUS "${target} UHTTP_LIBRARY_DIR: ${UHTTP_LIBRARY_DIR}")
    target_link_directories (${PROJECT_NAME} PRIVATE ${UHTTP_LIBRARY_DIR})
endif ()

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils
    PRIVATE aduc::adu_types
            aduc::logging
            Parson::parson
            uhttp)

if (ADUC_BUILD_UNIT_TESTS)
    target_link_umock_c (${PROJECT_NAME} PRIVATE)

    add_subdirectory (tests)
endif ()
