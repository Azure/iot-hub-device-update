cmake_minimum_required (VERSION 3.5)

set (target_name parser_utils)
add_library (${target_name} STATIC src/parser_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_TYPES_INCLUDES}
                                                  ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types Parson::parson
    PRIVATE aduc::hash_utils aduc::logging aduc::parson_json_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
