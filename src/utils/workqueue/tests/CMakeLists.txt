set (target_name workqueue_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp workqueue_unit_tests.cpp workqueue_worker_unit_tests.cpp)

target_link_libraries (${target_name} PRIVATE aduc::workqueue aduc::string_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
