cmake_minimum_required (VERSION 3.5)

project (logging)

add_library (${PROJECT_NAME} INTERFACE)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} INTERFACE inc)

set (ADUC_LOGGING_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if (ADUC_LOGGING_LIBRARY STREQUAL "xlog")
    # xlogging requires the azure iot shared util library.
    find_package (azure_c_shared_utility REQUIRED)
    target_link_libraries (${PROJECT_NAME} INTERFACE aziotsharedutil)
    target_compile_definitions (${PROJECT_NAME} INTERFACE ADUC_USE_XLOGGING=1)
elseif (ADUC_LOGGING_LIBRARY STREQUAL "zlog")
    add_subdirectory (zlog)
    target_link_libraries (${PROJECT_NAME} INTERFACE zlog)
    target_compile_definitions (${PROJECT_NAME} INTERFACE ADUC_USE_ZLOGGING=1)
else ()
    message (FATAL_ERROR "Unknown logging library ${ADUC_LOGGING_LIBRARY} specified.")
endif ()
