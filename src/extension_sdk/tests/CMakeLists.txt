cmake_minimum_required(VERSION 3.14)
project(extension_sdk_tests)

include(agentRules)

compileasc99()
disablertti()

find_package(Catch2 REQUIRED)
find_package(Parson REQUIRED)
find_package(OpenSSL REQUIRED)

set(test_sources
    comm_dispatcher_ut.cpp
    component_registry_ut.cpp
    config_reader_ut.cpp
    content_pipeline_ut.cpp
    dag_engine_ut.cpp
    download_tests_ut.cpp
    extension_loader_ut.cpp
    log_writer_ut.cpp
    manifest_parser_ut.cpp
    process_supervisor_ut.cpp
    security_ut.cpp
    v2_protocol_types_ut.cpp
    workflow_engine_ut.cpp)

add_executable(${PROJECT_NAME} ${test_sources})

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/../inc)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE aduc_extension_sdk
            tomlc99
            Parson::parson
            OpenSSL::Crypto
            Catch2::Catch2WithMain)

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread ${CMAKE_DL_LIBS})
endif()

include(CTest)
include(Catch)

if(NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests(${PROJECT_NAME} TEST_PREFIX "extension_sdk_")
else()
    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif()
