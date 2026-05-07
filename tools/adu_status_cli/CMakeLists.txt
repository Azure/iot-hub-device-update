cmake_minimum_required(VERSION 3.5)

project(adu_status_cli)

include(agentRules)

compileasc99()

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE aduc::aducsdk libaducpal)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif ()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
