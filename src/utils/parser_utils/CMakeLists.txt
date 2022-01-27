cmake_minimum_required (VERSION 3.5)

project (parser_utils)

add_library (${PROJECT_NAME} STATIC src/parser_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_TYPES_INCLUDES}
                                                   ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aziotsharedutil
           aduc::adu_core_interface
           aduc::c_utils
           aduc::hash_utils
           aduc::parson_json_utils
           aduc::system_utils
           Parson::parson
    PRIVATE aduc::logging aduc::string_utils)
