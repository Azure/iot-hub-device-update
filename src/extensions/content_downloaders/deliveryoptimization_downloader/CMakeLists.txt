set (target_name deliveryoptimization_content_downloader)
include (agentRules)

compileasc99 ()

add_library (${target_name} MODULE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (
    ${target_name}
    PRIVATE deliveryoptimization_content_downloader.cpp
            deliveryoptimization_content_downloader.EXPORTS.cpp
            deliveryoptimization_content_downloader.h)

target_include_directories (${target_name} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${target_name} PRIVATE)
target_link_dosdk (${target_name} PRIVATE)

target_link_libraries (${target_name} PRIVATE aduc::c_utils aduc::contract_utils aduc::hash_utils
                                              aduc::logging aduc::process_utils aduc::string_utils)

if (WIN32)
    target_link_libraries (${target_name} PRIVATE libaducpal)
else ()
    target_link_libraries (${target_name} PRIVATE Microsoft::deliveryoptimization atomic)
endif ()

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
