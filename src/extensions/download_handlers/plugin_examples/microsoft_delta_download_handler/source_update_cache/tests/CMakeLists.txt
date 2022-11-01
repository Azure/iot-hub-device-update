project (source_update_cache_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} "")

target_include_directories (${PROJECT_NAME} PRIVATE inc ${ADUC_EXPORT_INCLUDES})

target_sources (${PROJECT_NAME} PRIVATE main.cpp source_update_cache_utils_ut.cpp)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::file_utils
            aduc::source_update_cache
            aduc::test_utils
            aduc::system_utils
            aduc::workflow_utils
            Catch2::Catch2
            Parson::parson)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR="${ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR}"
)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
