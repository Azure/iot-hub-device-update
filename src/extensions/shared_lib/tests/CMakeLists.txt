cmake_minimum_required (VERSION 3.5)

project (shared_lib_unit_tests)

include (agentRules)

compileasc99 ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} shared_lib_ut.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::shared_lib
            Catch2::Catch2WithMain
            ${CMAKE_DL_LIBS})

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Export symbols from the test binary so dlsym(dlopen(""), ...) can find
# test functions used by CallExport unit tests.
target_link_options (${PROJECT_NAME} PRIVATE -rdynamic)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
