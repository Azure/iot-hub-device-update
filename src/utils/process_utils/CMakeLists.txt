cmake_minimum_required (VERSION 3.5)

set (target_name process_utils)
add_library (${target_name} STATIC src/process_utils.cpp)
add_library (aduc::${target_name} ALIAS ${target_name})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc)

find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aziotsharedutil
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::config_utils
            aduc::string_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
    add_subdirectory (test_helper)
endif ()
