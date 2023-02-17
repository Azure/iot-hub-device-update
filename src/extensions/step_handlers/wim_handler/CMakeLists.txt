cmake_minimum_required (VERSION 3.5)

project (microsoft_wim_1)

include (agentRules)
compileasc99 ()

add_library (${PROJECT_NAME} MODULE src/handler_create.cpp src/wim_handler_1.cpp
                                    src/wim_step_handler.cpp)

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
            aduc::workflow_data_utils
            aduc::workflow_utils
            aduc::logging)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

# TODO: Add tests
# if (ADUC_BUILD_UNIT_TESTS)
#     add_subdirectory (tests)
# endif ()
