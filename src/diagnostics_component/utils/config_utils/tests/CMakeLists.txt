project (diagnostics_config_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp diagnostics_config_utils_ut.cpp)

find_package (azure_c_shared_utility REQUIRED)
find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

target_compile_definitions (${target_name} PRIVATE ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")
add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aziotsharedutil
            diagnostic_utils::diagnostics_config_utils
            Catch2::Catch2
            Parson::parson)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
