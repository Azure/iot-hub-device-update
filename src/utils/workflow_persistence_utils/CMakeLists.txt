
cmake_minimum_required (VERSION 3.5)

project (workflow_persistence_utils)

include (agentRules)
compileasc99 ()

find_package (Parson REQUIRED)

add_library (${PROJECT_NAME} STATIC src/workflow_persistence_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_compile_definitions (${PROJECT_NAME}
    PUBLIC ADUC_WORKFLOW_PERSISTENCE_FILE_PATH="${ADUC_WORKFLOW_PERSISTENCE_FILE_PATH}"
           ADUC_BUILD_UNIT_TESTS="${ADUC_BUILD_UNIT_TESTS}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::logging
            aduc::system_utils
            aduc::workflow_utils
            aduc::parson_json_utils
            parson)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
