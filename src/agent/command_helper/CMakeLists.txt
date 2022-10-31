project (command_helper)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC ./src/command_helper.c )
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_COMMANDS_FIFO_NAME="${ADUC_COMMANDS_FIFO_NAME}"
    ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
    ADUC_FILE_USER="${ADUC_FILE_USER}"
)

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME}
    PRIVATE aduc::logging
            aduc::permission_utils )
