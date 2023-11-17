cmake_minimum_required (VERSION 3.5)

set (target_name rootkey_workflow)
include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC src/rootkey_workflow.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${target_name} PRIVATE)
find_package (Parson)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_ROOTKEY_STORE_PACKAGE_PATH="${ADUC_ROOTKEY_STORE_PACKAGE_PATH}"
            ADUC_ROOTKEY_STORE_PATH="${ADUC_ROOTKEY_STORE_PATH}"
            ADUC_ROOTKEY_PKG_URL_OVERRIDE="${ADUC_ROOTKEY_PKG_URL_OVERRIDE}")

include (CMakePrintHelpers)
cmake_print_variables (ADUC_ENABLE_E2E_TESTING)
if (ADUC_ENABLE_E2E_TESTING)
    cmake_print_variables (ADUC_ENABLE_E2E_TESTING)
    target_compile_definitions (${target_name} PRIVATE ADUC_E2E_TESTING_ENABLED="true")
endif ()

if (ADUC_ENABLE_SRVC_E2E_TESTING)
    cmake_print_variables (ADUC_ENABLE_SRVC_E2E_TESTING)
    target_compile_definitions (${target_name} PRIVATE ADUC_ENABLE_SRVC_E2E_TESTING="true")
endif ()

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::rootkeypackage_utils aduc::root_key_utils aduc::logging aduc::system_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
