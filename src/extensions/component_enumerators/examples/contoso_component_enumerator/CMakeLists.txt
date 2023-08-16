set (target_name contoso_component_enumerator)

include (agentRules)

compileasc99 ()

add_library (${target_name} MODULE contoso_component_enumerator.cpp)

find_package (Parson REQUIRED)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADU_EXPORT_INCLUDES})

target_link_libraries (${target_name} PUBLIC Parson::parson aduc::contract_utils)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
