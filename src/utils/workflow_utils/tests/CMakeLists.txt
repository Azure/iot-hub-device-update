cmake_minimum_required (VERSION 3.5)

project (workflow_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources} "")

target_sources (${PROJECT_NAME} PRIVATE main.cpp workflow_utils_ut.cpp
                                        workflow_get_update_file_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::parser_utils aduc::string_utils
                                               aduc::test_utils aduc::workflow_utils Catch2::Catch2 aduc::root_key_utils umock_c)

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
catch_discover_tests (${PROJECT_NAME})
