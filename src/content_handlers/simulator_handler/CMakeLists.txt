cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_simulator_1)

set (SOURCE_ALL src/simulator_handler.cpp)

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
    PUBLIC aduc::content_handlers
    PRIVATE aduc::c_utils
            aduc::exception_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
