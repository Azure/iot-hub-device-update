cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_apt_1)

set (SOURCE_ALL src/apt_handler.cpp src/apt_parser.cpp)

#
# Create a SHARED library.
#
add_library (${target_name} SHARED ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${ADUC_EXPORT_INCLUDES})

get_filename_component (
    ADUC_INSTALLEDCRITERIA_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}"
    ABSOLUTE
    "/")

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_link_libraries (
    ${target_name}
    PUBLIC aduc::logging
    PRIVATE aduc::adu_types
            aduc::contract_utils
            aduc::extension_manager
            aduc::installed_criteria_utils
            aduc::process_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

if (ADUC_BUILD_UNIT_TESTS)

    add_subdirectory (tests)

endif ()

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
