cmake_minimum_required (VERSION 3.5)

project (system_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources system_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::file_utils aduc::string_utils
                                               aduc::system_utils Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
