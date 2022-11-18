set (target_name rootkeypackage_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name} main.cpp rootkeypackage_utils_ut.cpp)

target_link_libraries (${target_name} PRIVATE aduc::rootkeypackage_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
