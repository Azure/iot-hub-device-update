cmake_minimum_required (VERSION 3.5)

project (workflow_data_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/workflow_data_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

# TODO(JeffMill): This is required for Windows to avoid
# C:\wiot-s1\src\utils\workflow_utils\inc\aduc/workflow_utils.h(15,10): fatal  error C1083:
# Cannot open include file: 'azure_c_shared_utility/strings.h': No such file or directory
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
endif ()

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_export_helpers
            aduc::adu_types
            aduc::c_utils
            aduc::workflow_utils)
