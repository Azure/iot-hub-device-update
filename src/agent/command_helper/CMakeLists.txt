set (target_name command_helper)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC ./src/command_helper.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_compile_definitions (
    ${target_name} PRIVATE ADUC_COMMANDS_FIFO_NAME="${ADUC_COMMANDS_FIFO_NAME}"
                           ADUC_FILE_GROUP="${ADUC_FILE_GROUP}" ADUC_FILE_USER="${ADUC_FILE_USER}")

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${target_name} PRIVATE aduc::logging aduc::permission_utils)
