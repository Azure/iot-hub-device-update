cmake_minimum_required (VERSION 3.5)

set (target_name loggingmanager)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

# Allow it to be used in a shared library
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_sources (${target_name} PRIVATE src/logging_manager.cpp)
target_link_libraries (${target_name} PUBLIC aduc::c_utils aduc::logging)
