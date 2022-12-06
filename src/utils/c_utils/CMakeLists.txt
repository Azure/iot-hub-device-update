cmake_minimum_required (VERSION 3.5)

project (c_utils)

include (agentRules)
compileasc99 ()

add_library (
    ${PROJECT_NAME} STATIC
    src/bit_ops.c
    src/connection_string_utils.c
    src/http_url.c
    src/string_c_utils.c)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c CONFIG REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)

    add_subdirectory (tests)
endif ()
