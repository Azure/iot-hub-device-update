set (target_name rootkeypackage_utils)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Parson REQUIRED)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources (${target_name} PRIVATE src/rootkeypackage_download.c src/rootkeypackage_parse.c
                                       src/rootkeypackage_utils.c)

if (ROOTKEY_PKG_DOWNLOAD_USE_CURL)
    target_sources (${target_name} PRIVATE src/rootkeypackage_curl_download.cpp)
else ()
    target_sources (${target_name} PRIVATE src/rootkeypackage_do_download.cpp)
endif ()

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${target_name} PUBLIC)
target_link_dosdk (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::hash_utils libaducpal
    PRIVATE aduc::crypto_utils aduc::logging aduc::system_utils aduc::url_utils Parson::parson)

if (ROOTKEY_PKG_DOWNLOAD_USE_CURL)
    target_link_libraries (${target_name} PRIVATE aduc::process_utils)
endif ()

target_compile_definitions (
    ${target_name} PRIVATE ADUC_ROOTKEY_PKG_URL_OVERRIDE="${ADUC_ROOTKEY_PKG_URL_OVERRIDE}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
