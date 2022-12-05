set (target_name rootkeypackage_utils)

include (find_curl_and_import_libcurl)
include (agentRules)
compileasc99 ()
disablertti ()

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)
find_package (deliveryoptimization_sdk CONFIG REQUIRED)
find_curl_and_import_libcurl ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (
    ${target_name}
    PRIVATE src/rootkeypackage_download.c
            src/rootkeypackage_parse.c
            src/rootkeypackage_utils.c
            src/rootkeypackage_curl_download.c
            src/rootkeypackage_do_download.cpp)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::hash_utils aziotsharedutil
    PRIVATE aduc::crypto_utils
            aduc::logging
            aduc::system_utils
            aduc::url_utils
            Parson::parson
            Microsoft::deliveryoptimization)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_DOWNLOADS_FOLDER="${ADUC_DOWNLOADS_FOLDER}"
                           ADUC_ROOTKEY_PKG_URL_OVERRIDE="${ADUC_ROOTKEY_PKG_URL_OVERRIDE}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
