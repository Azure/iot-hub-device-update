project (microsoft_delta_download_handler_util_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# ---------------------------------------------------------------
# Existing integration-style tests (use real workflow_utils)
# ---------------------------------------------------------------
add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE microsoft_delta_download_handler_utils_ut.cpp
                                        microsoft_delta_download_handler_utils_cpp_ut.cpp)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::microsoft_delta_download_handler_utils
            aduc::parser_utils
            aduc::workflow_utils
            Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()