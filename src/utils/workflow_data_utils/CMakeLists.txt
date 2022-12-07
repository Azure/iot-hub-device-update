cmake_minimum_required (VERSION 3.5)

project (workflow_data_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/workflow_data_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # TODO(JeffMill): This is required for Windows to avoid
    #  fatal  error C1083: Cannot open include file: 'azure_c_shared_utility/strings.h': No such file or directory
    find_package (azure_c_shared_utility CONFIG REQUIRED)
    target_include_directories (${PROJECT_NAME} PRIVATE ${AZURE_C_SHARED_UTILITY_INCLUDES})
endif ()

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_export_helpers
            aduc::adu_types
            aduc::c_utils
            aduc::workflow_utils)
