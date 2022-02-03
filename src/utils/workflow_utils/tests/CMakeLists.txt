cmake_minimum_required (VERSION 3.5)

project (workflow_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp workflow_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::parser_utils
            aduc::string_utils
            aduc::workflow_utils
            Catch2::Catch2
            aziotsharedutil)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
