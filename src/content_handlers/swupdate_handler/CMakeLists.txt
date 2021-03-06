cmake_minimum_required (VERSION 3.5)

set (target_name swupdate_handler)


if (ADUC_PLATFORM_LAYER STREQUAL "simulator")
    set (SOURCE_ALL src/swupdate_simulator_handler.cpp)
else ()
    set (SOURCE_ALL src/swupdate_handler.cpp)
endif ()

add_library (${target_name} STATIC ${SOURCE_ALL})

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc ${ADUC_EXPORT_INCLUDES} ${ADU_SHELL_INCLUDES})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::exception_utils)
target_compile_definitions (${target_name} PRIVATE ADUC_VERSION_FILE="${ADUC_VERSION_FILE}"
                                                   ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")

if (ADUC_BUILD_UNIT_TESTS AND ADUC_PLATFORM_LAYER STREQUAL "simulator")
    add_subdirectory (tests)
endif ()
