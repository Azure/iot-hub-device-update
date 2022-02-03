cmake_minimum_required (VERSION 3.5)

project (config_utils)

compileasc99 ()
add_library (${PROJECT_NAME} STATIC src/config_utils.c src/config_parsefile.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
                            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}")

if (ADUC_PLATFORM_LAYER STREQUAL "simulator")
    target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_PLATFORM_SIMULATOR)
endif ()

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils
    PRIVATE Parson::parson aziotsharedutil aduc::logging aduc::parson_json_utils)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)

    add_subdirectory (tests)
endif ()
