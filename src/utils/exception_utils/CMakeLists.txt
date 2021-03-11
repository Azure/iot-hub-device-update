cmake_minimum_required (VERSION 3.5)

set (target_name exception_utils)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${target_name} INTERFACE aduc::logging)
