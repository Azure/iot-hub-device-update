cmake_minimum_required (VERSION 3.5)

project (adu_types)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/adu_types.c)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package (Parson REQUIRED)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME} PUBLIC aduc::c_utils aduc::logging Parson::parson)
