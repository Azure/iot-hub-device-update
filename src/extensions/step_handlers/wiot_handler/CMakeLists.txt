cmake_minimum_required (VERSION 3.5)

project (microsoft_wiot_1)

include (agentRules)
compileasc99 ()

add_library (${PROJECT_NAME} MODULE src/handler_create.cpp src/wiot_handler_1.cpp
                                    src/step_handler.cpp)

# TODO: Bug 42997914: ADU_EXTENSION_INCLUDES is inconsistent with CMake patterns
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

# TODO: version.lib and system_utils are for demo code in step_handler.cpp. Remove when no longer in demo state.
target_link_libraries (${PROJECT_NAME} PRIVATE version aduc::system_utils)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

# TODO: Add tests
# if (ADUC_BUILD_UNIT_TESTS)
#     add_subdirectory (tests)
# endif ()
