cmake_minimum_required (VERSION 3.5)

project (microsoft_wiot_1)

include (agentRules)
compileasc99 ()

add_library (${PROJECT_NAME} SHARED src/handler_create.cpp src/wiot_handler_1.cpp
                                    src/step_handler.cpp)

# TODO(JeffMill): Bug 42997914: ADU_EXTENSION_INCLUDES is inconsistent with CMake patterns
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
            aduc::workflow_data_utils
            aduc::workflow_utils
            aduc::logging)

# TODO: version.lib is for demo code in step_handler.cpp. Remove.
target_link_libraries (${PROJECT_NAME} PRIVATE version)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

# TODO(JeffMill): Add tests
# if (ADUC_BUILD_UNIT_TESTS)
#     add_subdirectory (tests)
# endif ()
