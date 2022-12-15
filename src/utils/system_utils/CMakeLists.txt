cmake_minimum_required (VERSION 3.5)

project (system_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/system_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
                                                    ADUC_FILE_USER="${ADUC_FILE_USER}")

if (WIN32)
    # TODO(JeffMill): Without this, even when not building UTs, I get:
    # inc\aduc/http_url.h(19,10): fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
    target_link_umock_c (${PROJECT_NAME} PRIVATE)
endif ()

if (ADUC_BUILD_UNIT_TESTS)
    target_link_umock_c (${PROJECT_NAME} PRIVATE)

    add_subdirectory (tests)
endif ()
