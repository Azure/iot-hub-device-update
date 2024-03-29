cmake_minimum_required (VERSION 3.5)

project (installed_criteria_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_sources (${PROJECT_NAME} PRIVATE main.cpp installed_criteria_utils_ut.cpp)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${PROJECT_NAME} PRIVATE)
endif ()

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_TMP_DIR_PATH}/adu-test-installedcriteria")

target_link_libraries (
    ${PROJECT_NAME} PRIVATE aduc::adu_core_interface aduc::installed_criteria_utils
                            aduc::platform_layer Catch2::Catch2)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
