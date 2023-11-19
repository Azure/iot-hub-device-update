cmake_minimum_required (VERSION 3.5)

set (target_name hash_utils)
add_library (${target_name} STATIC src/hash_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PUBLIC)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::adu_types Parson::parson
    PRIVATE aduc::logging aduc::string_utils)

target_link_libraries (${target_name} PRIVATE libaducpal)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
