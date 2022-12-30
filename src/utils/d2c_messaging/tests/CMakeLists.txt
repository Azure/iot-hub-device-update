project (d2c_messaging_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_sources (${PROJECT_NAME} PRIVATE main.cpp d2c_messaging_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::communication_abstraction
            aduc::d2c_messaging
            aduc::retry_utils
            Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
