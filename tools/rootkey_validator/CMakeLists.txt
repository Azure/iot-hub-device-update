cmake_minimum_required (VERSION 3.5)

set (target_name rootkey_validator)

include (agentRules)

compileasc99 ()
disablertti ()

add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp)

# The rootkeypackage_utils library only compiles one downloader (curl or DO)
# based on the ROOTKEY_PKG_DOWNLOAD_USE_CURL flag. We want both available at
# runtime, so compile the other downloader source directly into this tool.
if (ROOTKEY_PKG_DOWNLOAD_USE_CURL)
    # rootkeypackage_utils already has curl download; add DO download
    target_sources (
        ${target_name}
        PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/rootkeypackage_utils/src/rootkeypackage_do_download.cpp)
else ()
    # rootkeypackage_utils already has DO download; add curl download
    target_sources (
        ${target_name}
        PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/rootkeypackage_utils/src/rootkeypackage_curl_download.cpp)
endif ()

target_include_directories (${target_name} PUBLIC ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${target_name} PRIVATE)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PRIVATE
        aduc::rootkeypackage_utils
        aduc::root_key_utils
        aduc::logging
        aduc::system_utils
        Parson::parson)

# Link process_utils for curl download (needed by ADUC_LaunchChildProcess)
target_link_libraries (${target_name} PRIVATE aduc::process_utils)

# Link DO SDK for DO download
target_link_dosdk (${target_name} PRIVATE)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_ROOTKEY_STORE_PACKAGE_PATH="${ADUC_ROOTKEY_STORE_PACKAGE_PATH}"
            ADUC_ROOTKEY_PKG_URL_OVERRIDE="${ADUC_ROOTKEY_PKG_URL_OVERRIDE}")