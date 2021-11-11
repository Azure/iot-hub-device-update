cmake_minimum_required (VERSION 3.5)

#
# Specify the update handler name.
# This should match the associated update type.
# e.g. 'microsoft/steps:1' ==> 'microsoft_steps_1'
#
set (target_name microsoft_steps_1)

#
# List all source files required for this handler.
#
set (SOURCE_ALL src/steps_handler.cpp)

#
# Create a shared library.
#
add_library (${target_name} SHARED ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADUC_TYPES_INCLUDES}
            ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADU_EXTENSION_INCLUDES})

get_filename_component (
    ADUC_INSTALLEDCRITERIA_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}"
    ABSOLUTE
    "/")

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::installed_criteria_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

#
# Copy a target file to out/bin/extensions/ directory.
#
set (OUT_BIN_EXTENTIONS_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/extensions)
file (MAKE_DIRECTORY ${OUT_BIN_EXTENTIONS_DIR})

add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -E copy
                ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*${target_name}.so
                ${OUT_BIN_EXTENTIONS_DIR})

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})