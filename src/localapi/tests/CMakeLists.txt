cmake_minimum_required(VERSION 3.5)

project(localapi_unit_tests)

include(agentRules)

compileasc99()
disablertti()

find_package(Catch2 REQUIRED)

# IPC Transport tests
add_executable(ipc_transport_ut ipc_transport_ut.cpp)

target_include_directories(
    ipc_transport_ut
    PRIVATE ${PROJECT_SOURCE_DIR}/../inc
            ${ADUC_EXPORT_INCLUDES})

target_link_libraries(
    ipc_transport_ut
    PRIVATE libaducpal
            Catch2::Catch2WithMain)

if (NOT WIN32)
    target_link_libraries(ipc_transport_ut PRIVATE pthread)
endif ()

# Local API server tests
add_executable(localapi_ut localapi_ut.cpp)

target_include_directories(
    localapi_ut
    PRIVATE ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../src
            ${ADUC_EXPORT_INCLUDES})

target_link_libraries(
    localapi_ut
    PRIVATE aduc::localapi
            libaducpal
            Catch2::Catch2WithMain)

if (NOT WIN32)
    target_link_libraries(localapi_ut PRIVATE pthread)
endif ()

# Register tests with CTest
include(CTest)
include(Catch)

if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests(ipc_transport_ut)
    catch_discover_tests(localapi_ut)
else ()
    add_test(NAME ipc_transport_ut COMMAND ipc_transport_ut)
    add_test(NAME localapi_ut COMMAND localapi_ut)
endif ()
