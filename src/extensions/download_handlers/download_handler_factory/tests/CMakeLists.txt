cmake_minimum_required (VERSION 3.5)

project (download_handler_factory_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE download_handler_factory_ut.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::extension_utils
            aduc::logging
            aduc::shared_lib
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
