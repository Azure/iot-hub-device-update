cmake_minimum_required (VERSION 3.5)

project (adu_shell_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} adushell_ut.cpp)

target_include_directories (
	${PROJECT_NAME}
	PUBLIC ../inc
		   ${ADUC_EXPORT_INCLUDES}
		   ${CMAKE_CURRENT_SOURCE_DIR}/../../logging/inc)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_USE_XLOGGING=1)

target_link_libraries (${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
