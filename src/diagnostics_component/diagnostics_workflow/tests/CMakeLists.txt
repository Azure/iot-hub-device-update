cmake_minimum_required (VERSION 3.5)

project (diagnostics_workflow_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp diagnostics_workflow_ut.cpp)

find_package (azure_c_shared_utility REQUIRED)
find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

target_compile_definitions (${target_name} PRIVATE ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")
add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2
            diagnostics_component::diagnostics_workflow
            Parson::parson
            aziotsharedutil)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
