project (workflow_data_utils_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME})
target_sources (${PROJECT_NAME} PRIVATE workflow_data_utils_ut.cpp)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
target_link_libraries (${PROJECT_NAME} PRIVATE aduc::workflow_data_utils aduc::workflow_utils 
                                               aduc::adu_types Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()