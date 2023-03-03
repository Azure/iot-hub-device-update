project (deliveryoptimization_content_downloader)

include (agentRules)

compileasc99 ()

find_package (deliveryoptimization_sdk CONFIG REQUIRED)

add_library (${PROJECT_NAME} MODULE)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (
    ${PROJECT_NAME}
    PRIVATE deliveryoptimization_content_downloader.cpp
            deliveryoptimization_content_downloader.EXPORTS.cpp
            deliveryoptimization_content_downloader.h)

target_include_directories (${PROJECT_NAME} PUBLIC ${ADU_EXTENSION_INCLUDES}
                                                   ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aziotsharedutil
            aduc::c_utils
            aduc::contract_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            Microsoft::deliveryoptimization)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
