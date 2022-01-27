cmake_minimum_required (VERSION 3.5)

project (system_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/system_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
find_package (azure_c_shared_utility REQUIRED)
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aziotsharedutil aduc::c_utils
    PRIVATE aduc::logging)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
            ADUC_FILE_USER="${ADUC_FILE_USER}")
            
if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
