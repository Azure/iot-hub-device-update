set (target_name rootkeypackage_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${target_name})
target_sources(${target_name} PRIVATE src/main.cpp src/rootkeypackage_utils_ut.cpp)

target_include_directories(${target_name} PRIVATE inc)

target_link_libraries (${target_name} PRIVATE aduc::rootkeypackage_utils aduc::test_utils aduc::crypto_utils
                                              Catch2::Catch2 Parson::parson)

target_compile_definitions (${target_name}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
