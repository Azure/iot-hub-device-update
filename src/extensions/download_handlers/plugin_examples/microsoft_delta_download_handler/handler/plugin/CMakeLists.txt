set (target_name microsoft-delta-download-handler)

include (agentRules)
compileasc99 ()

add_library (${target_name} MODULE "")

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/microsoft_delta_download_handler_plugin.c)

target_link_libraries (${target_name} PRIVATE aduc::adu_types
                                              aduc::microsoft_delta_download_handler aduc::logging)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
