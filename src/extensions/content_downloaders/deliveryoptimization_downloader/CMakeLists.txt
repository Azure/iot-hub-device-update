set (target_name deliveryoptimization_content_downloader)
include (agentRules)

compileasc99 ()

find_package (deliveryoptimization_sdk CONFIG REQUIRED)

add_library (${target_name} MODULE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (
    ${target_name}
    PRIVATE deliveryoptimization_content_downloader.cpp
            deliveryoptimization_content_downloader.EXPORTS.cpp
            deliveryoptimization_content_downloader.h)

target_include_directories (${target_name} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aziotsharedutil
            aduc::c_utils
            aduc::contract_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            atomic
            Microsoft::deliveryoptimization)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
