include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

# ==============================================================================
# Existing test target (links against real aduc::extension_manager)
# ==============================================================================
set (target_name extension_manager_tests)

add_executable (${target_name})

target_sources (${target_name} PRIVATE src/extension_manager_ut.cpp
                                       src/extension_manager_download_test_case.cpp
                                       src/extension_manager_helper_ut.cpp
                                       src/content_handler_ut.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                  ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::extension_manager
            aduc::config_utils
            aduc::entity_utils
            aduc::parser_utils
            aduc::string_utils
            aduc::system_utils
            aduc::test_utils
            aduc::workflow_utils
            Catch2::Catch2WithMain
            Parson::parson)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${target_name} PRIVATE)
endif ()

target_link_libraries (${target_name} PRIVATE libaducpal)

target_compile_definitions (${target_name} PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
catch_discover_tests (${target_name})

# ==============================================================================
# Mock-based test target for extension_manager.cpp
# Compiles extension_manager.cpp directly with mock dependencies.
# ==============================================================================
set (mock_em_target extension_manager_mock_tests)

add_executable (${mock_em_target})

target_sources (
    ${mock_em_target}
    PRIVATE
        src/extension_manager_mock_ut.cpp
        src/mock_extension_manager_deps.cpp
        # Compile the source under test directly
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/extension_manager.cpp)

target_include_directories (
    ${mock_em_target}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        # Include paths for headers used by the source (3 up from tests/ to src/)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../logging/zlog/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/config_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/contract_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/c_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/exception_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/extension_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/hash_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/parser_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/path_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/string_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/workflow_utils/inc
        # 2 up from tests/ to extensions/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../download_handlers/download_handler_factory/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../download_handlers/download_handler_plugin/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared_lib/inc)

target_compile_definitions (
    ${mock_em_target}
    PRIVATE
        ADUC_USE_ZLOGGING=1
        ADUC_CONF_FILE_PATH="/etc/adu/du-config.json"
        ADUC_EXTENSIONS_FOLDER="/var/lib/adu/extensions"
        ADUC_EXTENSIONS_INSTALL_FOLDER="/var/lib/adu/extensions"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="content_handler.json"
        ADUC_EXTENSION_REG_FILENAME="extension.json"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="update_content_handlers"
        ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER="content_downloader"
        ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR="component_enumerator")

target_link_libraries (
    ${mock_em_target}
    PRIVATE aduc::adu_types
            aduc::contract_utils
            aduc::c_utils
            aduc::string_utils
            Catch2::Catch2WithMain)

target_link_aziotsharedutil (${mock_em_target} PRIVATE)
target_link_libraries (${mock_em_target} PRIVATE libaducpal)

# Use --wrap to intercept POSIX calls (ADUCPAL_* macros expand to dlsym, access, etc. on Linux)
target_link_options (
    ${mock_em_target}
    PRIVATE
        -Wl,--wrap=dlopen
        -Wl,--wrap=dlerror
        -Wl,--wrap=dlsym
        -Wl,--wrap=dlclose
        -Wl,--wrap=access)

catch_discover_tests (${mock_em_target})

# ==============================================================================
# Mock-based test target for extension_manager_helper.cpp
# Compiles extension_manager_helper.cpp directly with mock dependencies.
# ==============================================================================
set (mock_helper_target extension_manager_helper_mock_tests)

add_executable (${mock_helper_target})

target_sources (
    ${mock_helper_target}
    PRIVATE
        src/extension_manager_helper_mock_ut.cpp
        src/mock_extension_manager_helper_deps.cpp
        # Compile the source under test directly
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/extension_manager_helper.cpp)

target_include_directories (
    ${mock_helper_target}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        # 3 up from tests/ to src/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../logging/zlog/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/config_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/contract_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/c_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/string_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/workflow_utils/inc
        # 2 up from tests/ to extensions/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../download_handlers/download_handler_factory/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../download_handlers/download_handler_plugin/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared_lib/inc)

target_compile_definitions (${mock_helper_target} PRIVATE ADUC_USE_ZLOGGING=1)

target_link_libraries (
    ${mock_helper_target}
    PRIVATE aduc::adu_types
            aduc::contract_utils
            aduc::c_utils
            Catch2::Catch2WithMain)

target_link_libraries (${mock_helper_target} PRIVATE libaducpal)

catch_discover_tests (${mock_helper_target})
