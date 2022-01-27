cmake_minimum_required (VERSION 3.5)

project (hash_utils)

add_library (${PROJECT_NAME} STATIC src/hash_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aziotsharedutil aduc::c_utils Parson::parson
    PRIVATE aduc::logging aduc::string_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
