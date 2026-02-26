cmake_minimum_required (VERSION 3.5)

project (download_handler_plugin_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# -----------------------------------------------------------------
# Build a minimal test plugin shared library that exports the 5
# required download-handler symbols.
# -----------------------------------------------------------------
set (test_plugin_target test_dh_plugin)
add_library (${test_plugin_target} SHARED test_download_handler_plugin_so.c)
target_include_directories (${test_plugin_target} PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})
target_link_libraries (${test_plugin_target} PRIVATE aduc::contract_utils)
set_target_properties (${test_plugin_target} PROPERTIES PREFIX "lib")

# -----------------------------------------------------------------
# Unit test executable
# -----------------------------------------------------------------
add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE download_handler_plugin_ut.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})

# Pass the path to the test plugin .so as a compile definition.
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE TEST_PLUGIN_SO_PATH="$<TARGET_FILE:${test_plugin_target}>")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::download_handler_plugin
            aduc::contract_utils
            aduc::logging
            aduc::shared_lib
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Ensure the test plugin is built before the test executable.
add_dependencies (${PROJECT_NAME} ${test_plugin_target})

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
