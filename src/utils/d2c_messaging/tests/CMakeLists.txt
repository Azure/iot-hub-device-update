project (d2c_messaging_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE d2c_messaging_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::communication_abstraction
            aduc::d2c_messaging
            aduc::retry_utils
            Catch2::Catch2WithMain)

include (CTest)
# Existing [functional]-tagged scenarios.
add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} [functional])
# ADO Bug 38069154: deterministic, non-functional tests for
# ADUC_D2C_Messaging_Reset_For_Handle_Refresh().
add_test (NAME ${PROJECT_NAME}_handle_refresh COMMAND ${PROJECT_NAME} [handle_refresh])
