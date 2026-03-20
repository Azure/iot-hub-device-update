set (target_name zlog_unit_test)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE zlog_ut.cpp)

target_compile_definitions (${target_name} PRIVATE ADUC_USE_ZLOGGING=1)

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            zlog)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
