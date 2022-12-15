project (command_helper)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC ./src/command_helper.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_COMMANDS_FIFO_NAME="${ADUC_COMMANDS_FIFO_NAME}"
                            ADUC_FILE_GROUP="${ADUC_FILE_GROUP}" ADUC_FILE_USER="${ADUC_FILE_USER}")

if (WIN32)
    # TODO(JeffMill): [PAL] pthread
    find_package (PThreads4W REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE PThreads4W::PThreads4W)
else ()
    # TODO(JeffMill): command_helper.c uses pthread but this isn't linking to threads?
    # find_package (Threads REQUIRED)
    # target_link_libraries (${PROJECT_NAME} PRIVATE Threads::Threads)
endif ()

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::logging aduc::permission_utils)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)
