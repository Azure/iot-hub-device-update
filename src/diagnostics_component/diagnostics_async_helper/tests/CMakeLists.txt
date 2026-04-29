project (diagnostics_async_helper_ut)

include (agentRules)

disablertti ()

set (sources diagnostics_async_helper_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE diagnostics_component::diagnostics_async_helper
            diagnostic_utils::diagnostics_config_utils
            aduc::logging
            Catch2::Catch2WithMain)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Wrap external dependency functions for mocking (GCC/ld only; not supported by MSVC)
if (NOT WIN32)
    target_link_options (
        ${PROJECT_NAME}
        PRIVATE
        -Wl,--wrap=DiagnosticsWorkflow_DiscoverAndUploadLogs
        -Wl,--wrap=OperationIdUtils_OperationIsComplete)
endif ()

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
