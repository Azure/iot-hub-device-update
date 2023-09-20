
set (target_name root_key_store_helper_ut)

include (agentRules)
compileasc99 ()
disablertti ()

add_executable (${target_name})

target_sources(${target_name} PRIVATE main.cpp root_key_store_helper_ut.cpp)

target_compile_definitions (${target_name} PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}" USE_LOCAL_STORE=1)

target_link_libraries (${target_name} PRIVATE aduc::rootkey_store aduc::root_key_utils aduc::rootkeypackage_utils aduc::string_utils)

find_package (Catch2 REQUIRED)
target_link_libraries (${target_name} PRIVATE Catch2::Catch2)

find_package (umock_c REQUIRED CONFIG)
target_link_libraries (${target_name} PRIVATE umock_c)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
