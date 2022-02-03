cmake_minimum_required (VERSION 3.5)

project (jws_utils)

compileasc99 ()
add_library (${PROJECT_NAME} STATIC src/jws_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::crypto_utils aduc::c_utils
    PRIVATE Parson::parson aziotsharedutil)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
