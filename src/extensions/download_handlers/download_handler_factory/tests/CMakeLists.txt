cmake_minimum_required (VERSION 3.5)

project (download_handler_factory_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# -----------------------------------------------------------------
# Build minimal test plugin shared libraries
# -----------------------------------------------------------------

# Full test plugin (Initialize + Cleanup)
set (test_plugin_target test_factory_plugin)
add_library (${test_plugin_target} SHARED test_factory_plugin.c)
set_target_properties (${test_plugin_target} PROPERTIES PREFIX "lib")

# Plugin without Initialize (triggers PluginException on load)
set (noinit_plugin_target test_factory_noinit_plugin)
add_library (${noinit_plugin_target} SHARED test_factory_noinit_plugin.c)
set_target_properties (${noinit_plugin_target} PROPERTIES PREFIX "lib")

# -----------------------------------------------------------------
# Unit test executable
# -----------------------------------------------------------------

add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE download_handler_factory_ut.cpp mock_factory_deps.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})

# Pass test plugin paths as compile definitions
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        TEST_FACTORY_PLUGIN_PATH="$<TARGET_FILE:${test_plugin_target}>"
        TEST_FACTORY_NOINIT_PLUGIN_PATH="$<TARGET_FILE:${noinit_plugin_target}>")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::logging
            aduc::shared_lib
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Ensure test plugins are built before the test executable.
add_dependencies (${PROJECT_NAME} ${test_plugin_target} ${noinit_plugin_target})

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
