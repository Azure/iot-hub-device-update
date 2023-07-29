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
    add_subdirectory (tests)
endif ()
