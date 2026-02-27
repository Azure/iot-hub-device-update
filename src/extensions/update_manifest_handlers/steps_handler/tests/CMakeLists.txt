cmake_minimum_required (VERSION 3.5)

project (steps_handler_mock_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

# ==============================================================================
# Mock-based test target for steps_handler.cpp and handler_create.cpp
# ==============================================================================

set (target_name steps_handler_mock_unit_tests)

add_executable (${target_name})

target_sources (
    ${target_name}
    PRIVATE
        steps_handler_mock_ut.cpp
        mock_steps_handler_deps.cpp
        # Compile sources under test directly
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/steps_handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler_create.cpp)

target_include_directories (
    ${target_name}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        ${ADU_SHELL_INCLUDES}
        # steps_handler own include dir
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        # extension_manager headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../extension_manager/inc
        # logging
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/zlog/inc
        # utility headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/c_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/config_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/contract_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/exception_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/extension_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/parser_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/string_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/system_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/workflow_utils/inc
        # download handler headers (component_enumerator_extension.hpp dependency chain)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_factory/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_plugin/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared_lib/inc)

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_INSTALLEDCRITERIA_FILE_PATH="/${ADUC_TMP_DIR_PATH}/adu-test-installedcriteria"
        ADUC_USE_ZLOGGING=1)

target_link_libraries (
    ${target_name}
    PRIVATE
        aduc::adu_types
        aduc::contract_utils
        aduc::c_utils
        Parson::parson
        Catch2::Catch2WithMain)

target_link_aziotsharedutil (${target_name} PRIVATE)
target_link_libraries (${target_name} PRIVATE libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})

# ==============================================================================
# Separate test target for handler_create.cpp exception paths.
# Compiles handler_create.cpp with a mock CreateContentHandler()
# (does NOT include steps_handler.cpp to avoid the real implementation).
# ==============================================================================

set (hc_target_name handler_create_mock_unit_tests)

add_executable (${hc_target_name})

target_sources (
    ${hc_target_name}
    PRIVATE
        handler_create_mock_ut.cpp
        mock_handler_create_deps.cpp
        # Compile only handler_create.cpp (the file under test)
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler_create.cpp)

target_include_directories (
    ${hc_target_name}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        # steps_handler own include dir
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        # logging
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/zlog/inc
        # utility headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/c_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/contract_utils/inc)

target_compile_definitions (
    ${hc_target_name}
    PRIVATE
        ADUC_USE_ZLOGGING=1)

target_link_libraries (
    ${hc_target_name}
    PRIVATE
        aduc::c_utils
        Catch2::Catch2WithMain)

target_link_libraries (${hc_target_name} PRIVATE libaducpal)

catch_discover_tests (${hc_target_name})
