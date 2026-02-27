cmake_minimum_required (VERSION 3.5)

project (agent_workflow_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE main.cpp agent_workflow_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Catch2::Catch2
            aduc::agent_workflow
            aduc::adu_types
            aduc::c_utils
            aduc::config_utils
            aduc::test_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            aduc::agent_orchestration
            umock_c)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${PROJECT_NAME} PRIVATE)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
