cmake_minimum_required (VERSION 3.5)

project (agent_orchestration_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE main.cpp agent_orchestration_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Catch2::Catch2
            aduc::agent_orchestration
            aduc::adu_types)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${PROJECT_NAME} PRIVATE)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
