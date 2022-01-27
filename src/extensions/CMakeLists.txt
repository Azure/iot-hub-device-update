set (ADUC_EXTENSION_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_subdirectory (extension_manager)
add_subdirectory (content-downloaders)
add_subdirectory (component-enumerators/examples/contoso-component-enumerator)
