cmake_minimum_required (VERSION 3.5)

set (target_name string_utils)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/string_utils.cpp)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
