cmake_minimum_required (VERSION 3.5)

project (microsoft_delta_download_handler_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE microsoft_delta_download_handler_ut.cpp mock_delta_deps.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::microsoft-delta-download-handler
            Catch2::Catch2WithMain)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
