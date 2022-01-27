cmake_minimum_required (VERSION 3.5)

project (extension_manager)

add_library (${PROJECT_NAME} STATIC src/extension_manager.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                   ${ADU_EXTENSION_INCLUDES})

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        ADUC_EXTENSIONS_FOLDER="${ADUC_EXTENSIONS_FOLDER}"
        ADUC_EXTENSIONS_INSTALL_FOLDER="${ADUC_EXTENSIONS_INSTALL_FOLDER}"
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME="${ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME}"
        ADUC_EXTENSION_REG_FILENAME="${ADUC_EXTENSION_REG_FILENAME}"
        ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS="${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
        ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER="${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}"
        ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR="${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}"
        ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR="${ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR}"
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR="${ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR}"
        ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR="${ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR}")

#
# Note: add ${CMAKE_DL_LIBS} for dynamic library loading support.
#
target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::extension_utils
    PRIVATE aduc::c_utils
            aduc::content_handlers
            aduc::exception_utils
            aduc::string_utils
            aduc::logging
            ${CMAKE_DL_LIBS})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (${ADUC_PLATFORM_LAYER} STREQUAL "simulator")
    target_compile_definitions (${PROJECT_NAME} PUBLIC ADUC_SIMULATOR_MODE=1)
endif ()
