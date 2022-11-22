cmake_minimum_required (VERSION 3.5)

project (root_key_utils)

add_library (${PROJECT_NAME} STATIC src/root_key_util.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

find_package (azure_c_shared_utility REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils aduc::crypto_utils aziotsharedutil
    PRIVATE
           aduc::rootkeypackage_utils)

# Always support test root keys.
add_definitions (-DBUILD_WITH_TEST_KEYS=1)

target_compile_definitions(${PROJECT_NAME} PRIVATE ADUC_ROOTKEY_STORE_PATH)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
