cmake_minimum_required (VERSION 3.5)

set (target_name communication_abstraction_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})

target_sources (${target_name} PRIVATE client_handle_helper_ut.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            Catch2::Catch2
            aduc::communication_abstraction
            aduc::adu_types
            aduc::c_utils
            aduc::logging
            Parson::parson)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_umock_c (${target_name} PRIVATE)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()
