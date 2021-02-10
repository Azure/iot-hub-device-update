cmake_minimum_required (VERSION 3.5)

project (process_utils)

add_library (${PROJECT_NAME} STATIC src/process_utils.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::logging aduc::c_utils aduc::string_utils)

if ((NOT
     ${ADUC_PLATFORM_LAYER}
     STREQUAL
     "simulator"
    )
    AND ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
    add_subdirectory (test_helper)
endif ()
