cmake_minimum_required (VERSION 3.5)

set (target_name logging)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc)

set (ADUC_LOGGING_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if (ADUC_LOGGING_LIBRARY STREQUAL "xlog")
    # xlogging requires the azure iot shared util library.
    target_link_aziotsharedutil (${target_name} INTERFACE)
    target_compile_definitions (${target_name} INTERFACE ADUC_USE_XLOGGING=1)
elseif (ADUC_LOGGING_LIBRARY STREQUAL "zlog")
    add_subdirectory (zlog)
    target_link_libraries (${target_name} INTERFACE zlog)
    target_compile_definitions (${target_name} INTERFACE ADUC_USE_ZLOGGING=1)
else ()
    message (FATAL_ERROR "Unknown logging library ${ADUC_LOGGING_LIBRARY} specified.")
endif ()
