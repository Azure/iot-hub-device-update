cmake_minimum_required (VERSION 3.5)

project (workflow_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/workflow_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE src)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC  aduc::c_utils
    PRIVATE aduc::adu_types
            aduc::hash_utils
            aduc::jws_utils
            aduc::logging
            aduc::parser_utils
            aduc::system_utils
            Parson::parson)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
