project (contoso_component_enumerator)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} SHARED contoso_component_enumerator.cpp)

find_package (Parson REQUIRED)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADU_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME} PUBLIC Parson::parson aduc::contract_utils)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
