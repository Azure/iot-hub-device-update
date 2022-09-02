project (curl_content_downloader)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} SHARED)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (
    ${PROJECT_NAME} PRIVATE curl_content_downloader.cpp curl_content_downloader.EXPORTS.cpp
                            curl_content_downloader.h)

target_include_directories (${PROJECT_NAME} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADU_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::contract_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
