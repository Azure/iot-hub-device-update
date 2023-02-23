project (azure_iot_aduc_agent)
set (target_name download_utils_unit_tests)

project (download_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp download_utils_unit_tests.cpp)

target_link_libraries (${target_name} PRIVATE aduc::download_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
