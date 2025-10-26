cmake_minimum_required(VERSION 3.5)

project(apiproto_utils_unit_tests)

include(agentRules)
compileasc99()
disablertti()

set(sources apiproto_utils_ut.cpp)

find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}
    PRIVATE aduc::apiproto_utils
            aduc::c_utils
            aduc::logging
            Catch2::Catch2WithMain
            pthread)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE ADUC_ALLOW_UNSIGNED_UPDATES
            ADUC_PLATFORM_LINUX)

# Code coverage configuration
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE --coverage -g -O0)
        target_link_options(${PROJECT_NAME} PRIVATE --coverage)

        # Find coverage tools
        find_program(GCOV_TOOL gcov)
        find_program(LCOV_TOOL lcov)
        find_program(GENHTML_TOOL genhtml)

        if(LCOV_TOOL AND GENHTML_TOOL)
            # Add custom target for coverage report
            add_custom_target(apiproto_coverage
                COMMAND ${CMAKE_COMMAND} -E remove_directory apiproto_coverage_output
                COMMAND ${CMAKE_COMMAND} -E make_directory apiproto_coverage_output
                COMMAND ${LCOV_TOOL} --capture --initial --directory ${CMAKE_SOURCE_DIR}/src/utils/apiproto_utils --output-file apiproto_coverage_output/baseline.info
                COMMAND ${CMAKE_CTEST_COMMAND} -R ${PROJECT_NAME} --output-on-failure
                COMMAND ${LCOV_TOOL} --capture --directory ${CMAKE_SOURCE_DIR}/src/utils/apiproto_utils --output-file apiproto_coverage_output/test.info
                COMMAND ${LCOV_TOOL} --add-tracefile apiproto_coverage_output/baseline.info --add-tracefile apiproto_coverage_output/test.info --output-file apiproto_coverage_output/total.info
                COMMAND ${LCOV_TOOL} --remove apiproto_coverage_output/total.info '/usr/*' '*/tests/*' '*/build/*' '*/deps/*' '*catch2*' --output-file apiproto_coverage_output/filtered.info
                COMMAND ${GENHTML_TOOL} apiproto_coverage_output/filtered.info --output-directory apiproto_coverage_output/html --title "APIProto Utils Coverage Report"
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in apiproto_coverage_output/html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PROJECT_NAME}
                COMMENT "Generating apiproto_utils code coverage report"
            )

            # Add a simpler coverage target that just shows percentage
            add_custom_target(apiproto_coverage_summary
                COMMAND ${CMAKE_COMMAND} -E echo "Running tests to generate coverage data..."
                COMMAND ${CMAKE_CTEST_COMMAND} -R ${PROJECT_NAME} --output-on-failure
                COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage summary..."
                COMMAND ${LCOV_TOOL} --capture --directory . --output-file coverage.info
                COMMAND ${LCOV_TOOL} --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' '*/deps/*' '*catch2*' --output-file coverage_filtered.info
                COMMAND ${LCOV_TOOL} --list coverage_filtered.info
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PROJECT_NAME}
                COMMENT "Showing apiproto_utils coverage summary"
            )
        else()
            message(STATUS "lcov and genhtml not found - coverage targets not available")
        endif()
    else()
        message(WARNING "Code coverage only supported with GCC or Clang")
    endif()
endif()

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
