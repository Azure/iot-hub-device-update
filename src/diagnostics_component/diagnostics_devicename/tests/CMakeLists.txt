project (diagnostics_devicename_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources diagnostics_devicename_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE diagnostics_component::diagnostics_devicename
                                               Catch2::Catch2WithMain)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()