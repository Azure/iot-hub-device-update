project (diagnostics_result_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources diagnostics_result_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE diagnostics_component::diagnostics_workflow
                                               Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()

# -------------------------------------------------------------------
# diagnostics_workflow_ut - tests for diagnostics_workflow.c functions
# -------------------------------------------------------------------
project (diagnostics_workflow_ut)

compileasc99 ()
disablertti ()

set (workflow_ut_sources diagnostics_workflow_ut.cpp)

find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${workflow_ut_sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE diagnostics_component::diagnostics_workflow
            aduc::logging
            Catch2::Catch2WithMain
            Parson::parson)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Wrap external dependency functions for mocking
target_link_options (
    ${PROJECT_NAME}
    PRIVATE
    -Wl,--wrap=FileUploadUtility_UploadFilesToContainer
    -Wl,--wrap=FileInfoUtils_GetNewestFilesInDirUnderSize
    -Wl,--wrap=DiagnosticsInterface_ReportStateAndResultAsync
    -Wl,--wrap=OperationIdUtils_StoreCompletedOperationId
    -Wl,--wrap=DiagnosticsComponent_GetDeviceName
    -Wl,--wrap=mallocAndStrcpy_s
    -Wl,--wrap=STRING_new_with_memory
    -Wl,--wrap=STRING_new
    -Wl,--wrap=STRING_sprintf)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
