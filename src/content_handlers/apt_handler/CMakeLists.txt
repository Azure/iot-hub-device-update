cmake_minimum_required (VERSION 3.5)

set (target_name apt_handler)

if (ADUC_PLATFORM_LAYER STREQUAL "simulator")
    set (SOURCE_ALL src/apt_simulator_handler.cpp)
else ()
    set (SOURCE_ALL src/apt_handler.cpp src/apt_parser.cpp)
endif ()

add_library (${target_name} STATIC ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc ${ADUC_EXPORT_INCLUDES} ${ADU_SHELL_INCLUDES})

get_filename_component (
    ADUC_INSTALLEDCRITERIA_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}"
    ABSOLUTE
    "/")

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_link_libraries (
    ${target_name}
    PRIVATE aduc::apt_handler
            aduc::c_utils
            aduc::logging
            aduc::process_utils
            aduc::system_utils
            aduc::string_utils
            aduc::exception_utils
            Parson::parson)

if (ADUC_BUILD_UNIT_TESTS AND ADUC_PLATFORM_LAYER STREQUAL "linux")
    add_subdirectory (tests)
endif ()
