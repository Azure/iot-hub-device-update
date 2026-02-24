cmake_minimum_required (VERSION 3.5)

project (pnp_helper_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} pnp_protocol_ut.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::pnp_helper
            aduc::logging
            Catch2::Catch2WithMain
            Parson::parson
            IotHubClient::iothub_client)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
