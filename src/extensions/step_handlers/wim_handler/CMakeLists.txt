cmake_minimum_required (VERSION 3.5)

project (microsoft_wim_1)

include (agentRules)
compileasc99 ()

add_library (
    ${PROJECT_NAME} MODULE
    src/bcdedit.cpp
    src/file_version.cpp
    src/format_drive.cpp
    src/handler_create.cpp
    src/wim_handler_1.cpp
    src/wim_step_handler.cpp
    src/wimg.cpp)

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${ADU_EXTENSION_INCLUDES} ${ADUC_TYPES_INCLUDES})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::c_utils
            aduc::contract_utils
            aduc::extension_manager
            aduc::parser_utils
            aduc::process_utils
            aduc::shutdown_service
            aduc::workflow_data_utils
            aduc::workflow_utils
            aduc::logging)

# GetFileVersionInfo()
target_link_libraries (${PROJECT_NAME} PRIVATE Version.lib)

# Wimgapi
# TODO: Bring in Wimgapi in a better way, rather than storing SDK in repo.
target_include_directories (${PROJECT_NAME} PRIVATE Wimgapi/Include)
target_link_directories (${PROJECT_NAME} PRIVATE Wimgapi/lib/${CMAKE_SYSTEM_PROCESSOR})
target_link_libraries (${PROJECT_NAME} PRIVATE wimgapi.lib)

# WBEM
target_link_libraries (${PROJECT_NAME} PRIVATE Wbemuuid.lib)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
