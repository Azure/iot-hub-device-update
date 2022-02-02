project (microsoft_delta_download_handler_util_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE main.cpp microsoft_delta_download_handler_utils_ut.cpp)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::microsoft_delta_download_handler_utils
            aduc::workflow_utils
            Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
