
set (target_name crypto_test_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp crypto_test_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${target_name} ${sources})

target_link_libraries (${target_name} PRIVATE aduc::test_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
