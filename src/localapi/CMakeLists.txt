cmake_minimum_required(VERSION 3.5)

project(localapi)

include(agentRules)

compileasc99()
disablertti()

add_library(${PROJECT_NAME} STATIC)
add_library(aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(
    ${PROJECT_NAME}
    PRIVATE src/localapi_server.c
            src/localapi_security.c)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC inc
           ${ADUC_EXPORT_INCLUDES}
    PRIVATE src)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils
    PRIVATE libaducpal)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif ()

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif ()
