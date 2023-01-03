cmake_minimum_required (VERSION 3.5)

project (process_utils)

add_library (${PROJECT_NAME} STATIC src/process_utils.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::config_utils
            aduc::string_utils)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

if (WIN32)
    # TODO(JeffMill): Without this, even when not building UTs, I get:
    # inc\aduc/http_url.h(19,10): fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
    target_link_umock_c (${PROJECT_NAME} PRIVATE)
endif ()

if ((NOT
     ${ADUC_PLATFORM_LAYER}
     STREQUAL
     "simulator"
    )
    AND ADUC_BUILD_UNIT_TESTS)

    add_subdirectory (tests)
    add_subdirectory (test_helper)
endif ()
