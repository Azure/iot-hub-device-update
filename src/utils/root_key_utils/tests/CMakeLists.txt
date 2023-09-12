cmake_minimum_required (VERSION 3.5)

set (target_name root_key_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()


find_package (Catch2 REQUIRED)
find_package (umock_c REQUIRED CONFIG)

set (sources main.cpp root_key_utils_ut.cpp)

add_executable (${target_name})

target_sources(${target_name} PRIVATE main.cpp root_key_utils_ut.cpp root_key_utils_load_package_ut.cpp root_key_utils_load_package_ut.cpp)

target_compile_definitions (${target_name} PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}" USE_LOCAL_STORE=1)

target_link_libraries (${target_name} PRIVATE aduc::root_key_utils aduc::rootkeypackage_utils aduc::string_utils Catch2::Catch2 umock_c)


include (CTest)
include (Catch)
catch_discover_tests (${target_name})
