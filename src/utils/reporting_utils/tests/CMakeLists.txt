set (target_name reporting_utils_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp reporting_utils_ut.cpp)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::reporting_utils
            aduc::string_utils
            aziotsharedutil
            Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
