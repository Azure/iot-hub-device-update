cmake_minimum_required (VERSION 3.5)

project (adu_core_export_helpers_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} adu_core_export_helpers_ut.cpp
                                ${CMAKE_SOURCE_DIR}/src/agent/adu_core_export_helpers/src/adu_core_export_helpers.c)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/agent/adu_core_export_helpers/inc
            ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Parson::parson
            aduc::c_utils
            aduc::adu_types)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
