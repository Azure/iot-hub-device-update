cmake_minimum_required (VERSION 3.5)

set (target_name installed_criteria_utils)
add_library (${target_name} STATIC src/installed_criteria_utils.cpp)
add_library (aduc::${target_name} ALIAS ${target_name})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${target_name} PRIVATE aduc::adu_core_interface aduc::logging Parson::parson)

target_link_libraries (${target_name} PRIVATE libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
