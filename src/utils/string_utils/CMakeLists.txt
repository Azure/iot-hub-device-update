cmake_minimum_required (VERSION 3.5)

project (string_utils)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/string_utils.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
