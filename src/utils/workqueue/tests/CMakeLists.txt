project (workqueue_unit_tests)

include (agentRules)

disablertti ()

set (sources workqueue_unit_tests.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::workqueue aduc::string_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
