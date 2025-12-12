cmake_minimum_required (VERSION 3.5)

project (device_properties_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

# Add the test executable
add_executable (${PROJECT_NAME} device_properties_ut.cpp
                                ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/src/device_properties.c)

# Include directories
target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/src
            ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/inc
            ${ADUC_EXPORT_INCLUDES})

# Link libraries
target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Parson::parson
            aduc::c_utils
            aduc::config_utils
            aduc::logging
            aduc::string_utils)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Compile definitions needed by device_properties.c
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_DEVICEPROPERTIES_MANUFACTURER="${ADUC_DEVICEPROPERTIES_MANUFACTURER}"
            ADUC_DEVICEPROPERTIES_MODEL="${ADUC_DEVICEPROPERTIES_MODEL}"
            ADUC_VERSION="${ADUC_VERSION}"
            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}")

# Enable testing
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
