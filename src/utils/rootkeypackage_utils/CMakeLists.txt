cmake_minimum_required (VERSION 3.5)

project (rootkeypackage_utils)

compileasc99 ()

find_package (azure_c_shared_utility REQUIRED)

add_library (${PROJECT_NAME} STATIC src/rootkeypackage_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME}
    PUBLIC aziotsharedutil
    PRIVATE aduc::c_utils aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
