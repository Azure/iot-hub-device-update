cmake_minimum_required (VERSION 3.5)

project (curl_content_downloader_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

# Compile curl_content_downloader.cpp and EXPORTS.cpp source directly (parent is MODULE lib, can't link to it).
target_sources (${PROJECT_NAME} PRIVATE curl_content_downloader_ut.cpp
                                        ../curl_content_downloader.cpp
                                        ../curl_content_downloader.EXPORTS.cpp)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/..
            ${ADUC_EXPORT_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${CMAKE_SOURCE_DIR}/src/utils/process_utils/inc)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::contract_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
