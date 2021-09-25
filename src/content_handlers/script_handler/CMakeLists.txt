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
            aduc::workflow_utils)


#if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)

    set (OUT_BIN_TESTFILES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/script-handler-testfiles)
    file (MAKE_DIRECTORY ${OUT_BIN_TESTFILES_DIR}) 

    #
    # Copy all files from ./tests/files directory to out/bin directory.
    #
    add_custom_command(TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/files ${OUT_BIN_TESTFILES_DIR}
    )

    set (MOCK_SANDBOX_DIR /tmp/adu_script_handle_tests/sandbox1/bundle)
    file (MAKE_DIRECTORY ${MOCK_SANDBOX_DIR}) 
    file (MAKE_DIRECTORY ${MOCK_SANDBOX_DIR}/leaf-0) 

    #
    # Copy some test files from ./tests/files directory to mock sandbox directory.
    #
    add_custom_command(TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/files/leaf-0 ${MOCK_SANDBOX_DIR}/leaf-0
    )

#endif ()

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})