cmake_minimum_required (VERSION 3.5)

project (shutdown_service_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} shutdown_service_ut.cpp)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::shutdown_service Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
