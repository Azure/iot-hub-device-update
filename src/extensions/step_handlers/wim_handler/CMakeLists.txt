cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_wim_1)

include (agentRules)
compileasc99 ()

add_library (
    ${target_name} MODULE
    src/bcdedit.cpp
    src/file_version.cpp
    src/format_drive.cpp
    src/handler_create.cpp
    src/wim_handler_1.cpp
    src/wim_step_handler.cpp
    src/wimg.cpp)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${ADU_EXTENSION_INCLUDES} ${ADUC_TYPES_INCLUDES})

target_link_aziotsharedutil (${target_name} PRIVATE)

# Windows needs all DLLS to have links to all potential libraries so this is included here
if (WIN32)

    target_link_dosdk (${target_name} PRIVATE)

endif ()

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::contract_utils
            aduc::extension_manager
            aduc::parser_utils
            aduc::process_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            aduc::logging)

# GetFileVersionInfo()
target_link_libraries (${target_name} PRIVATE Version.lib)

# Wimgapi
# TODO: Bring in Wimgapi in a better way, rather than storing SDK in repo.
target_include_directories (${target_name} PRIVATE Wimgapi/Include)
target_link_directories (${target_name} PRIVATE Wimgapi/lib/${CMAKE_SYSTEM_PROCESSOR})
target_link_libraries (${target_name} PRIVATE wimgapi.lib)

# WBEM
target_link_libraries (${target_name} PRIVATE Wbemuuid.lib)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
