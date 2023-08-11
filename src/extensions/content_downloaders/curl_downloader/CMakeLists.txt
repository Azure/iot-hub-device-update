set (target_name curl_content_downloader)
include (agentRules)

compileasc99 ()

add_library (${target_name} MODULE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (
    ${target_name} PRIVATE curl_content_downloader.cpp curl_content_downloader.EXPORTS.cpp
                           curl_content_downloader.h)

target_include_directories (${target_name} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADU_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::contract_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils)

target_link_libraries (${target_name} PRIVATE libaducpal)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
