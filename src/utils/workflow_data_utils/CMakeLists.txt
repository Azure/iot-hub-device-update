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

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_export_helpers
            aduc::adu_types
            aduc::c_utils
            aduc::workflow_utils)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
