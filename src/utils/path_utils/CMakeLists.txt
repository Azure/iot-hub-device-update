project (path_utils)

add_library (${PROJECT_NAME} STATIC "")
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE src/path_utils.c)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (${PROJECT_NAME} PUBLIC aduc::c_utils)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)

    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        # TODO(JeffMill): This is required for Windows to avoid
        # fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
        target_include_directories (${PROJECT_NAME} PRIVATE ${UMOCK_C_INC_FOLDER})
    endif ()

    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)

    add_subdirectory (tests)
endif ()
