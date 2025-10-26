#!/bin/bash

# create_test_infrastructure.sh
# Script to bootstrap unit testing infrastructure for ADU components
#
# Usage: ./create_test_infrastructure.sh <component_path> [component_type]
# Example: ./create_test_infrastructure.sh src/utils/my_utils util
#          ./create_test_infrastructure.sh src/agent/my_agent critical

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 <component_path> [component_type]"
    echo ""
    echo "Arguments:"
    echo "  component_path    Path to the component (e.g., src/utils/my_utils)"
    echo "  component_type    Type of component: util|critical|platform|extension (default: util)"
    echo ""
    echo "Examples:"
    echo "  $0 src/utils/string_utils util"
    echo "  $0 src/agent/api critical"
    echo "  $0 src/platform_layers/linux platform"
    echo "  $0 src/extensions/my_handler extension"
    echo ""
    echo "Component Types:"
    echo "  util      - Utility library (85-90% coverage target)"
    echo "  critical  - Critical component (90-95% coverage target)"
    echo "  platform  - Platform layer (80-90% coverage target)"
    echo "  extension - Extension/handler (75-85% coverage target)"
}

# Check arguments
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    show_usage
    exit 1
fi

COMPONENT_PATH="$1"
COMPONENT_TYPE="${2:-util}"

# Validate component type
case "$COMPONENT_TYPE" in
util | critical | platform | extension) ;;

*)
    print_error "Invalid component type: $COMPONENT_TYPE"
    show_usage
    exit 1
    ;;
esac

# Validate component path exists
if [ ! -d "$COMPONENT_PATH" ]; then
    print_error "Component path does not exist: $COMPONENT_PATH"
    exit 1
fi

# Extract component name from path
COMPONENT_NAME=$(basename "$COMPONENT_PATH")
TEST_DIR="$COMPONENT_PATH/tests"

print_status "Setting up test infrastructure for: $COMPONENT_NAME"
print_status "Component type: $COMPONENT_TYPE"
print_status "Target directory: $TEST_DIR"

# Create tests directory if it doesn't exist
if [ ! -d "$TEST_DIR" ]; then
    mkdir -p "$TEST_DIR"
    print_success "Created tests directory: $TEST_DIR"
else
    print_warning "Tests directory already exists: $TEST_DIR"
fi

# Set coverage targets based on component type
case "$COMPONENT_TYPE" in
critical)
    LINE_COVERAGE="90-95"
    FUNCTION_COVERAGE="100"
    ;;
util)
    LINE_COVERAGE="85-90"
    FUNCTION_COVERAGE="95-100"
    ;;
platform)
    LINE_COVERAGE="80-90"
    FUNCTION_COVERAGE="90-95"
    ;;
extension)
    LINE_COVERAGE="75-85"
    FUNCTION_COVERAGE="85-90"
    ;;
esac

# Generate CMakeLists.txt for tests
TEST_CMAKE_FILE="$TEST_DIR/CMakeLists.txt"
if [ ! -f "$TEST_CMAKE_FILE" ]; then
    cat > "$TEST_CMAKE_FILE" << EOF
cmake_minimum_required(VERSION 3.5)

project(${COMPONENT_NAME}_unit_tests)

include(agentRules)
compileasc99()
disablertti()

# Test sources
set(sources ${COMPONENT_NAME}_ut.cpp)

# Find dependencies
find_package(Catch2 REQUIRED)

# Create test executable
add_executable(\${PROJECT_NAME} \${sources})

# Link libraries
target_link_libraries(\${PROJECT_NAME}
    PRIVATE aduc::${COMPONENT_NAME}
            Catch2::Catch2WithMain)

# Add additional dependencies based on component type
# Uncomment and modify as needed:
# target_link_libraries(\${PROJECT_NAME} PRIVATE aduc::c_utils)
# target_link_libraries(\${PROJECT_NAME} PRIVATE aduc::logging)
# target_link_libraries(\${PROJECT_NAME} PRIVATE libaducpal)

# Test data configuration
target_compile_definitions(\${PROJECT_NAME}
    PRIVATE ADUC_TEST_DATA_FOLDER="\${ADUC_TEST_DATA_FOLDER}")

# Coverage configuration
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(\${PROJECT_NAME} PRIVATE --coverage -g -O0)
        target_link_options(\${PROJECT_NAME} PRIVATE --coverage)

        # Find coverage tools
        find_program(LCOV_TOOL lcov)
        find_program(GENHTML_TOOL genhtml)

        if(LCOV_TOOL AND GENHTML_TOOL)
            # Add coverage report target
            add_custom_target(\${PROJECT_NAME}_coverage
                COMMAND \${CMAKE_COMMAND} -E remove_directory \${PROJECT_NAME}_coverage_output
                COMMAND \${CMAKE_COMMAND} -E make_directory \${PROJECT_NAME}_coverage_output
                COMMAND \${LCOV_TOOL} --capture --initial --directory \${CMAKE_SOURCE_DIR}/${COMPONENT_PATH} --output-file \${PROJECT_NAME}_coverage_output/baseline.info
                COMMAND \${CMAKE_CTEST_COMMAND} -R \${PROJECT_NAME} --output-on-failure
                COMMAND \${LCOV_TOOL} --capture --directory \${CMAKE_SOURCE_DIR}/${COMPONENT_PATH} --output-file \${PROJECT_NAME}_coverage_output/test.info
                COMMAND \${LCOV_TOOL} --add-tracefile \${PROJECT_NAME}_coverage_output/baseline.info --add-tracefile \${PROJECT_NAME}_coverage_output/test.info --output-file \${PROJECT_NAME}_coverage_output/total.info
                COMMAND \${LCOV_TOOL} --remove \${PROJECT_NAME}_coverage_output/total.info '/usr/*' '*/tests/*' '*/build/*' '*/deps/*' '*catch2*' --output-file \${PROJECT_NAME}_coverage_output/filtered.info
                COMMAND \${GENHTML_TOOL} \${PROJECT_NAME}_coverage_output/filtered.info --output-directory \${PROJECT_NAME}_coverage_output/html --title "${COMPONENT_NAME} Coverage Report"
                COMMAND \${CMAKE_COMMAND} -E echo "Coverage report: \${PROJECT_NAME}_coverage_output/html/index.html"
                WORKING_DIRECTORY \${CMAKE_BINARY_DIR}
                DEPENDS \${PROJECT_NAME}
                COMMENT "Generating ${COMPONENT_NAME} coverage report (Target: ${LINE_COVERAGE}% lines, ${FUNCTION_COVERAGE}% functions)"
            )

            # Add coverage summary target
            add_custom_target(\${PROJECT_NAME}_coverage_summary
                COMMAND \${CMAKE_COMMAND} -E echo "Running ${COMPONENT_NAME} tests for coverage analysis..."
                COMMAND \${CMAKE_CTEST_COMMAND} -R \${PROJECT_NAME} --output-on-failure
                COMMAND \${CMAKE_COMMAND} -E echo "Generating coverage summary..."
                COMMAND \${LCOV_TOOL} --capture --directory . --output-file coverage.info
                COMMAND \${LCOV_TOOL} --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' '*/deps/*' '*catch2*' --output-file coverage_filtered.info
                COMMAND \${LCOV_TOOL} --list coverage_filtered.info
                COMMAND \${CMAKE_COMMAND} -E echo "Coverage target: ${LINE_COVERAGE}% lines, ${FUNCTION_COVERAGE}% functions"
                WORKING_DIRECTORY \${CMAKE_BINARY_DIR}
                DEPENDS \${PROJECT_NAME}
                COMMENT "Showing ${COMPONENT_NAME} coverage summary"
            )
        else()
            message(STATUS "lcov and genhtml not found - coverage targets not available")
        endif()
    else()
        message(WARNING "Code coverage only supported with GCC or Clang")
    endif()
endif()

# Test discovery
include(CTest)
include(Catch)
catch_discover_tests(\${PROJECT_NAME})
EOF
    print_success "Created CMakeLists.txt: $TEST_CMAKE_FILE"
else
    print_warning "CMakeLists.txt already exists: $TEST_CMAKE_FILE"
fi

# Generate test file template
TEST_FILE="$TEST_DIR/${COMPONENT_NAME}_ut.cpp"
if [ ! -f "$TEST_FILE" ]; then
    # Find header files to include
    HEADER_FILE=""
    if [ -f "$COMPONENT_PATH/inc/${COMPONENT_NAME}.h" ]; then
        HEADER_FILE="${COMPONENT_NAME}/${COMPONENT_NAME}.h"
    elif [ -f "$COMPONENT_PATH/inc/aduc/${COMPONENT_NAME}.h" ]; then
        HEADER_FILE="aduc/${COMPONENT_NAME}.h"
    else
        HEADER_FILE="${COMPONENT_NAME}/${COMPONENT_NAME}.h  // TODO: Adjust include path"
    fi

    cat > "$TEST_FILE" << EOF
/**
 * @file ${COMPONENT_NAME}_ut.cpp
 * @brief Unit tests for ${COMPONENT_NAME}
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "${HEADER_FILE}"
#include "aduc/c_utils.h"

#include <catch2/catch_all.hpp>
#include <string>

using Catch::Matchers::Equals;

/**
 * @brief Test fixture for ${COMPONENT_NAME} testing
 *
 * Coverage Target: ${LINE_COVERAGE}% lines, ${FUNCTION_COVERAGE}% functions
 */
class ${COMPONENT_NAME^}TestFixture
{
public:
    ${COMPONENT_NAME^}TestFixture()
    {
        // Setup test environment
        // Initialize test data, create temporary files, etc.
    }

    ~${COMPONENT_NAME^}TestFixture()
    {
        // Cleanup test environment
        // Remove temporary files, reset global state, etc.
    }

protected:
    // Helper methods for test setup
    void setup_test_data()
    {
        // Create test data files, initialize test state
    }

    void cleanup_test_data()
    {
        // Clean up test artifacts
    }

    // Test data paths
    const std::string test_data_folder = ADUC_TEST_DATA_FOLDER;
};

TEST_CASE_METHOD(${COMPONENT_NAME^}TestFixture, "${COMPONENT_NAME} Happy Path Tests")
{
    SECTION("Basic functionality")
    {
        // TODO: Implement basic functionality tests
        // Example:
        // auto result = ${COMPONENT_NAME}_function(valid_input);
        // REQUIRE(result == expected_output);

        WARN("TODO: Implement basic functionality tests");
    }

    SECTION("Normal operation with valid inputs")
    {
        // TODO: Test normal operation scenarios
        // Test various valid input combinations
        // Verify expected outputs and side effects

        WARN("TODO: Implement normal operation tests");
    }

    SECTION("Edge cases within valid range")
    {
        // TODO: Test edge cases
        // Boundary values, empty inputs, maximum sizes, etc.

        WARN("TODO: Implement edge case tests");
    }
}

TEST_CASE_METHOD(${COMPONENT_NAME^}TestFixture, "${COMPONENT_NAME} Error Handling Tests")
{
    SECTION("Null pointer handling")
    {
        // TODO: Test null pointer inputs
        // Example:
        // auto result = ${COMPONENT_NAME}_function(nullptr);
        // REQUIRE(result == ERROR_INVALID_PARAMETER);

        WARN("TODO: Implement null pointer tests");
    }

    SECTION("Invalid input validation")
    {
        // TODO: Test invalid inputs
        // Out of range values, malformed data, etc.

        WARN("TODO: Implement invalid input tests");
    }

    SECTION("Resource exhaustion handling")
    {
        // TODO: Test resource limits
        // Memory allocation failures, file handle limits, etc.

        WARN("TODO: Implement resource exhaustion tests");
    }
}

TEST_CASE("${COMPONENT_NAME} Constants and Configuration")
{
    SECTION("Constant validation")
    {
        // TODO: Validate compile-time constants
        // Ensure constants have expected values
        // Verify relationships between constants

        WARN("TODO: Implement constant validation tests");
    }

    SECTION("Configuration parameter validation")
    {
        // TODO: Test configuration handling
        // Default values, parameter validation, etc.

        WARN("TODO: Implement configuration tests");
    }
}

TEST_CASE_METHOD(${COMPONENT_NAME^}TestFixture, "${COMPONENT_NAME} Cross-Platform Compatibility")
{
    SECTION("Platform-specific behavior")
    {
#ifdef _WIN32
        // TODO: Windows-specific tests
        WARN("TODO: Implement Windows-specific tests");
#elif defined(__linux__)
        // TODO: Linux-specific tests
        WARN("TODO: Implement Linux-specific tests");
#endif
    }

    SECTION("Cross-platform consistency")
    {
        // TODO: Test behavior that should be consistent across platforms
        // Data format handling, algorithm results, etc.

        WARN("TODO: Implement cross-platform consistency tests");
    }
}

// TODO: Add component-specific test cases based on functionality
// Examples:
// - Performance/benchmark tests for critical operations
// - Concurrency tests for thread-safe components
// - Integration tests with dependent components
// - Stress tests for resource-intensive operations

/*
 * Test Implementation Guidelines:
 *
 * 1. Remove WARN statements and implement actual tests
 * 2. Add specific test cases based on component functionality
 * 3. Ensure coverage of all public API functions
 * 4. Test error conditions and edge cases thoroughly
 * 5. Use descriptive test names and section labels
 * 6. Add performance tests for critical operations
 * 7. Mock external dependencies appropriately
 * 8. Validate memory management (no leaks)
 * 9. Test thread safety if applicable
 * 10. Document complex test scenarios
 *
 * Coverage Target: ${LINE_COVERAGE}% lines, ${FUNCTION_COVERAGE}% functions
 */
EOF
    print_success "Created test file template: $TEST_FILE"
else
    print_warning "Test file already exists: $TEST_FILE"
fi

# Update component CMakeLists.txt to include tests
COMPONENT_CMAKE="$COMPONENT_PATH/CMakeLists.txt"
if [ -f "$COMPONENT_CMAKE" ]; then
    # Check if test subdirectory is already added
    if ! grep -q "add_subdirectory.*tests" "$COMPONENT_CMAKE"; then
        # Add coverage support and test subdirectory
        cat >> "$COMPONENT_CMAKE" << EOF

# Enable coverage for this library when building tests
if (ADUC_BUILD_UNIT_TESTS AND ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${COMPONENT_NAME} PRIVATE --coverage -g -O0)
    endif()
endif()

# Test subdirectory
if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif()
EOF
        print_success "Updated component CMakeLists.txt to include tests"
    else
        print_warning "Component CMakeLists.txt already has test support"
    fi
else
    print_warning "Component CMakeLists.txt not found: $COMPONENT_CMAKE"
fi

# Create test data directory if needed
TEST_DATA_DIR="$TEST_DIR/testdata"
if [ ! -d "$TEST_DATA_DIR" ]; then
    mkdir -p "$TEST_DATA_DIR"

    # Create sample test data file
    cat > "$TEST_DATA_DIR/sample_data.txt" << EOF
Sample test data for ${COMPONENT_NAME}
This file can be used for testing file operations, parsing, etc.
EOF

    print_success "Created test data directory: $TEST_DATA_DIR"
fi

# Create README for the test directory
TEST_README="$TEST_DIR/README.md"
if [ ! -f "$TEST_README" ]; then
    cat > "$TEST_README" << EOF
# ${COMPONENT_NAME} Unit Tests

## Overview
Unit tests for the ${COMPONENT_NAME} component.

**Coverage Target**: ${LINE_COVERAGE}% lines, ${FUNCTION_COVERAGE}% functions

## Building and Running Tests

### Build Tests
\`\`\`bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON ..
make ${COMPONENT_NAME}_unit_tests
\`\`\`

### Run Tests
\`\`\`bash
# Run tests
./${TEST_DIR}/${COMPONENT_NAME}_unit_tests

# Run with verbose output
./${TEST_DIR}/${COMPONENT_NAME}_unit_tests -v

# Run specific test case
./${TEST_DIR}/${COMPONENT_NAME}_unit_tests -t "test_case_name"
\`\`\`

### Code Coverage

#### Generate Coverage Report
\`\`\`bash
# Build with coverage
cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON -DENABLE_COVERAGE=ON ..
make ${COMPONENT_NAME}_unit_tests

# Generate HTML coverage report
make ${COMPONENT_NAME}_unit_tests_coverage

# View coverage summary
make ${COMPONENT_NAME}_unit_tests_coverage_summary
\`\`\`

#### Coverage Targets
- **Line Coverage**: ${LINE_COVERAGE}%
- **Function Coverage**: ${FUNCTION_COVERAGE}%

## Test Structure

### Test Files
- \`${COMPONENT_NAME}_ut.cpp\` - Main unit tests
- \`testdata/\` - Test data files

### Test Categories
1. **Happy Path Tests** - Normal operation scenarios
2. **Error Handling Tests** - Invalid inputs and error conditions
3. **Edge Case Tests** - Boundary conditions and limits
4. **Cross-Platform Tests** - Platform-specific behavior validation

## Implementation Guidelines

1. **Test Coverage**: Aim for ${LINE_COVERAGE}% line coverage and ${FUNCTION_COVERAGE}% function coverage
2. **Error Testing**: Test all error conditions and invalid inputs
3. **Null Safety**: Verify null pointer handling
4. **Resource Management**: Test memory and resource cleanup
5. **Cross-Platform**: Validate behavior across different platforms
6. **Performance**: Add benchmark tests for critical operations

## Adding New Tests

1. Add test cases to appropriate sections in \`${COMPONENT_NAME}_ut.cpp\`
2. Use descriptive test names and SECTION labels
3. Follow the existing test fixture pattern
4. Add test data files to \`testdata/\` as needed
5. Update this README with any new test categories

## Debugging Tests

### Running Tests in Debugger
\`\`\`bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON ..
make ${COMPONENT_NAME}_unit_tests

# Run in gdb
gdb ./${TEST_DIR}/${COMPONENT_NAME}_unit_tests
\`\`\`

### Verbose Test Output
\`\`\`bash
# Show all test output
./${TEST_DIR}/${COMPONENT_NAME}_unit_tests -s

# Show test durations
./${TEST_DIR}/${COMPONENT_NAME}_unit_tests -d yes
\`\`\`
EOF
    print_success "Created test README: $TEST_README"
fi

# Generate build and test script
BUILD_SCRIPT="$TEST_DIR/build_and_test.sh"
if [ ! -f "$BUILD_SCRIPT" ]; then
    cat > "$BUILD_SCRIPT" << 'EOF'
#!/bin/bash

# Build and test script for component testing
# Usage: ./build_and_test.sh [coverage|debug|release]

set -e

MODE="${1:-debug}"
COMPONENT_NAME=$(basename "$(dirname "$(pwd)")")
BUILD_DIR="../../../build"

case "$MODE" in
    coverage)
        echo "Building with coverage..."
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON -DENABLE_COVERAGE=ON ..
        make "${COMPONENT_NAME}_unit_tests" -j$(nproc)

        echo "Running tests with coverage..."
        make "${COMPONENT_NAME}_unit_tests_coverage_summary"
        ;;
    debug)
        echo "Building in debug mode..."
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON ..
        make "${COMPONENT_NAME}_unit_tests" -j$(nproc)

        echo "Running tests..."
        "./${COMPONENT_NAME}_unit_tests"
        ;;
    release)
        echo "Building in release mode..."
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release -DADUC_BUILD_UNIT_TESTS=ON ..
        make "${COMPONENT_NAME}_unit_tests" -j$(nproc)

        echo "Running tests..."
        "./${COMPONENT_NAME}_unit_tests"
        ;;
    *)
        echo "Usage: $0 [coverage|debug|release]"
        exit 1
        ;;
esac
EOF
    chmod +x "$BUILD_SCRIPT"
    print_success "Created build script: $BUILD_SCRIPT"
fi

print_success "Test infrastructure setup complete for $COMPONENT_NAME!"
echo ""
print_status "Next steps:"
echo "1. Edit $TEST_FILE to implement actual test cases"
echo "2. Remove WARN statements and add real assertions"
echo "3. Add component-specific dependencies to $TEST_CMAKE_FILE"
echo "4. Create test data files in $TEST_DATA_DIR as needed"
echo "5. Build and run tests:"
echo "   cd build"
echo "   cmake -DCMAKE_BUILD_TYPE=Debug -DADUC_BUILD_UNIT_TESTS=ON .."
echo "   make ${COMPONENT_NAME}_unit_tests"
echo "   ./${TEST_DIR}/${COMPONENT_NAME}_unit_tests"
echo ""
print_status "Coverage targets for $COMPONENT_TYPE components:"
echo "   Line Coverage: ${LINE_COVERAGE}%"
echo "   Function Coverage: ${FUNCTION_COVERAGE}%"
echo ""
print_status "Generate coverage reports with:"
echo "   cmake -DENABLE_COVERAGE=ON .."
echo "   make ${COMPONENT_NAME}_unit_tests_coverage"
