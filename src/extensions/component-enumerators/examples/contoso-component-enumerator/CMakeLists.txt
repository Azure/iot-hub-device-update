project (contoso-component-enumerator)

include (agentRules)

compileasc99 ()

add_library (
    ${PROJECT_NAME} SHARED
    contoso-component-enumerator.cpp
)

find_package (Parson REQUIRED)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC ${ADUC_EXTENSION_INCLUDES} ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC Parson::parson)

#
# Copy a target file to out/bin/extensions/ directory.
#
set (OUT_BIN_EXTENTIONS_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/extensions)
file (MAKE_DIRECTORY ${OUT_BIN_EXTENTIONS_DIR})

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -E copy 
                ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*${PROJECT_NAME}.so
                ${OUT_BIN_EXTENTIONS_DIR})

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
