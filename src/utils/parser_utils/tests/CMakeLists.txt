project (parser_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources} "")

target_sources (${PROJECT_NAME} PRIVATE parser_utils_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::hash_utils aduc::parser_utils aduc::string_utils Catch2::Catch2WithMain)

target_compile_definitions (${PROJECT_NAME}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
