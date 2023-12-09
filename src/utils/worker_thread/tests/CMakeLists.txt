set (target_name worker_thread_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE aduc_worker_thread_unit_tests.cpp)
target_link_libraries (${target_name} PRIVATE aduc::worker_thread Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
