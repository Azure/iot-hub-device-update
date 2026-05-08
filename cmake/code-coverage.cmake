# code-coverage.cmake
# Adds code coverage support via gcov + lcov
#
# Usage:
#   cmake -DADUC_ENABLE_COVERAGE=ON ...
#   ninja
#   ctest
#   ninja coverage  # generates HTML report
#
# Requires: gcov, lcov, genhtml

option(ADUC_ENABLE_COVERAGE "Enable code coverage with gcov/lcov" OFF)

if(ADUC_ENABLE_COVERAGE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Code coverage requires Debug build type for accurate results")
    endif()

    # Check for required tools
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(NOT GCOV_PATH OR NOT LCOV_PATH OR NOT GENHTML_PATH)
        message(FATAL_ERROR "Coverage requires gcov, lcov, and genhtml. Install with: apt-get install lcov")
    endif()

    # Add coverage flags to all targets
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")

    # Coverage target
    add_custom_target(coverage
        COMMENT "Generating code coverage report..."

        # Reset counters
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters

        # Run tests
        COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure

        # Capture coverage data
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture
                --output-file ${CMAKE_BINARY_DIR}/coverage.info
                --gcov-tool ${GCOV_PATH}

        # Filter: only include our source
        COMMAND ${LCOV_PATH} --extract ${CMAKE_BINARY_DIR}/coverage.info
                '${CMAKE_SOURCE_DIR}/src/extension_sdk/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/step_handlers/script_handler_v2/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/step_handlers/apt_handler_v2/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/step_handlers/swupdate_handler_v2/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/communication_providers/adu_direct/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/component_enumerators/static_file/src/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/content_downloaders/curl_downloader_v2/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/content_downloaders/file_sideloader/*'
                '${CMAKE_SOURCE_DIR}/src/extensions/communication_providers/simulator_comm/*'
                --output-file ${CMAKE_BINARY_DIR}/coverage_filtered.info

        # Remove test files from coverage
        COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage_filtered.info
                '*/tests/*' '*/test/*' '*_ut.cpp' '*_test.cpp'
                --output-file ${CMAKE_BINARY_DIR}/coverage_final.info

        # Generate HTML report
        COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage_final.info
                --output-directory ${CMAKE_BINARY_DIR}/coverage_report
                --title "ADU Gen2 Code Coverage"
                --legend --show-details
                --highlight --demangle-cpp

        # Print summary
        COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage_final.info

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Coverage check target (fails if below threshold)
    add_custom_target(coverage_check
        COMMENT "Checking coverage threshold (85%)..."
        COMMAND ${CMAKE_COMMAND} -E echo "Checking coverage >= 85%..."
        COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage_final.info 2>&1
                | grep -oP "lines.*?:\\s*\\K[0-9.]+"
                | ${CMAKE_COMMAND} -E env python3 -c
                "import sys; pct=float(sys.stdin.read().strip()); print(f'Coverage: {pct}%'); sys.exit(0 if pct >= 85.0 else 1)"
        DEPENDS coverage
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    message(STATUS "Code coverage enabled (gcov + lcov)")
    message(STATUS "  Run 'ninja coverage' after building to generate report")
    message(STATUS "  Run 'ninja coverage_check' to verify 85% threshold")
endif()
