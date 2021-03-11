cmake_minimum_required (VERSION 3.5)

project (installed_criteria_utils)

add_library (${PROJECT_NAME} STATIC src/installed_criteria_utils.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}")

target_include_directories (${PROJECT_NAME} PUBLIC inc 
                ${ADUC_EXPORT_INCLUDES} )

target_link_libraries (${PROJECT_NAME}
        PRIVATE aduc::logging 
                aduc::c_utils 
                aduc::string_utils
                Parson::parson
                )


if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
