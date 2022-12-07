project (agent_workflow)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC "")

set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE src/agent_workflow.c)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

find_package (Parson REQUIRED)

# TODO(JeffMill): This is required for Windows to avoid
# fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)
endif ()

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
    PRIVATE aduc::adu_core_export_helpers
            aduc::agent_orchestration
            aduc::c_utils
            aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::logging
            aduc::parser_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

# TODO(JeffMill): Removed this. Unclear when needed (ARM builds?)
# target_link_libraries(${PROJECT_NAME} PRIVATE -zdef)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}")
