project (diagnostics_interface_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources diagnostics_interface_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE diagnostics_component::diagnostics_interface
            aduc::d2c_messaging
            aduc::logging
            aduc::pnp_helper
            libaducpal
            Catch2::Catch2WithMain
            Parson::parson)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Wrap external dependency functions for mocking
target_link_options (
    ${PROJECT_NAME}
    PRIVATE
    -Wl,--wrap=PnP_CreateReportedProperty
    -Wl,--wrap=PnP_CreateReportedPropertyWithStatus
    -Wl,--wrap=ADUC_D2C_Message_SendAsync
    -Wl,--wrap=DiagnosticsConfigUtils_InitFromFile
    -Wl,--wrap=DiagnosticsConfigUtils_UnInit
    -Wl,--wrap=DiagnosticsWorkflow_DiscoverAndUploadLogsAsync)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
