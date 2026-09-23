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

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}"
        ADUC_CONTENT_DOWNLOADER_EXTENSION_PATH="${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}/${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_COMPONENT_ENUMERATOR_EXTENSION_PATH="${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}/${ADUC_EXTENSION_REG_FILENAME}")

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()