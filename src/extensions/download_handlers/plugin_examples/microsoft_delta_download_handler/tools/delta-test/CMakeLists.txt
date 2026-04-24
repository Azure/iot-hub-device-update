cmake_minimum_required (VERSION 3.5)

set (target_name adu-delta-test)

include (agentRules)

compileasc99 ()

add_executable (${target_name} src/main.c)

target_include_directories (
    ${target_name}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../handler/lib/inc)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::file_utils
            aduc::hash_utils
            aduc::microsoft-delta-download-handler
            aduc::workflow_utils)

install (TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
