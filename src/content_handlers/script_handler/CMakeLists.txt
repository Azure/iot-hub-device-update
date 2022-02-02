cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_script_1)

set (SOURCE_ALL src/script_handler.cpp)

set (SCRIPT_HANDLER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library (${target_name} SHARED ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADUC_TYPES_INCLUDES}
            ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::exception_utils
            aduc::extension_utils
            aduc::extension_manager
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            -zdefs
            )

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
