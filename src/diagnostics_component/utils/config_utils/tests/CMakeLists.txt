project (diagnostics_config_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources diagnostics_config_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")

target_link_libraries (${PROJECT_NAME} PRIVATE diagnostic_utils::diagnostics_config_utils
                                               Catch2::Catch2WithMain Parson::parson)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()