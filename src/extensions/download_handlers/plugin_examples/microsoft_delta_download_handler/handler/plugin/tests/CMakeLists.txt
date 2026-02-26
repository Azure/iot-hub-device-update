cmake_minimum_required (VERSION 3.5)

project (microsoft_delta_download_handler_exports_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_sources (
    ${PROJECT_NAME}
    PRIVATE
        microsoft_delta_download_handler_exports_ut.cpp
        mock_exports_deps.cpp
        # Compile the source under test directly (can't link MODULE libraries)
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/microsoft_delta_download_handler_plugin.EXPORTS.c)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        # Include path for aduc/microsoft_delta_download_handler.h (from handler/lib)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/inc
        # Include paths for aduc/logging.h and zlog.h (mocked, so don't link aduc::logging)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../logging/zlog/inc)

# Need ADUC_USE_ZLOGGING so logging.h resolves correctly (but we mock the functions)
target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_USE_ZLOGGING=1)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::contract_utils
            aduc::c_utils
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
