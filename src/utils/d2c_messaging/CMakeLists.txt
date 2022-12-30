cmake_minimum_required (VERSION 3.5)

project (d2c_messaging)

add_library (${PROJECT_NAME} STATIC src/d2c_messaging.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC ./inc ${ADUC_TYPES_INCLUDES}
                                                   ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
    PRIVATE aduc::communication_abstraction
            aduc::logging
            aduc::retry_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
