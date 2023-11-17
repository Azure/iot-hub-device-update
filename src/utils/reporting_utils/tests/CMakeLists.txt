project (reporting_utils_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME})
target_sources (${PROJECT_NAME} PRIVATE main.cpp reporting_utils_ut.cpp)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
target_link_libraries (${PROJECT_NAME} PRIVATE aduc::c_utils aduc::reporting_utils
                                               aduc::string_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
