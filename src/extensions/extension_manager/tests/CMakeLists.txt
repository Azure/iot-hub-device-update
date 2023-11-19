set (target_name extension_manager_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${target_name})

target_sources (${target_name} PRIVATE src/main.cpp src/extension_manager_ut.cpp
                                       src/extension_manager_download_test_case.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                  ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::extension_manager
            aduc::entity_utils
            aduc::parser_utils
            aduc::string_utils
            aduc::system_utils
            aduc::test_utils
            aduc::workflow_utils
            Catch2::Catch2
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
