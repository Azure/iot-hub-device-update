#!/bin/bash

# analyze_test_coverage.sh
# Script to analyze test coverage across all ADU components
#
# Usage: ./analyze_test_coverage.sh [--detailed] [--missing-only] [--component-type TYPE]

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Command line options
DETAILED=false
MISSING_ONLY=false
COMPONENT_TYPE=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    --detailed)
        DETAILED=true
        shift
        ;;
    --missing-only)
        MISSING_ONLY=true
        shift
        ;;
    --component-type)
        COMPONENT_TYPE="$2"
        shift 2
        ;;
    --help | -h)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --detailed         Show detailed analysis of each component"
        echo "  --missing-only     Show only components missing test infrastructure"
        echo "  --component-type   Filter by component type (util|critical|platform|extension)"
        echo "  --help, -h         Show this help message"
        echo ""
        echo "Component Types:"
        echo "  util      - Utility libraries in src/utils/"
        echo "  critical  - Critical components (agent, communication)"
        echo "  platform  - Platform abstraction layers"
        echo "  extension - Extension and handler components"
        exit 0
        ;;
    *)
        echo "Unknown option: $1"
        exit 1
        ;;
    esac
done

# Function to print colored output
print_header() {
    echo -e "${CYAN}=================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}=================================${NC}"
}

print_section() {
    echo -e "${BLUE}--- $1 ---${NC}"
}

print_component() {
    echo -e "${GREEN}✓${NC} $1"
}

print_missing() {
    echo -e "${RED}✗${NC} $1"
}

print_partial() {
    echo -e "${YELLOW}~${NC} $1"
}

# Function to classify component type
classify_component() {
    local path="$1"

    if [[ $path =~ ^src/utils/ ]]; then
        echo "util"
    elif [[ $path =~ ^src/(agent|communication_|sdk) ]]; then
        echo "critical"
    elif [[ $path =~ ^src/(platform_layers|libaducpal) ]]; then
        echo "platform"
    elif [[ $path =~ ^src/(extensions|diagnostics_component) ]]; then
        echo "extension"
    else
        echo "other"
    fi
}

# Function to analyze component test status
analyze_component() {
    local comp_path="$1"
    local comp_name
    comp_name=$(basename "$comp_path")
    local comp_type
    comp_type=$(classify_component "$comp_path")

    # Skip if filtering by component type
    if [[ -n $COMPONENT_TYPE && $comp_type != "$COMPONENT_TYPE" ]]; then
        return
    fi

    local has_tests=false
    local has_cmake=false
    local has_test_files=false
    local cmake_has_tests=false
    local test_count=0

    # Check for tests directory
    if [[ -d "$comp_path/tests" ]]; then
        has_tests=true

        # Check for CMakeLists.txt in tests
        if [[ -f "$comp_path/tests/CMakeLists.txt" ]]; then
            has_cmake=true
        fi

        # Count test files
        test_count=$(find "$comp_path/tests" -name "*_ut.cpp" -o -name "*_test.cpp" | wc -l)
        if [[ $test_count -gt 0 ]]; then
            has_test_files=true
        fi
    fi

    # Check if main CMakeLists.txt includes tests
    if [[ -f "$comp_path/CMakeLists.txt" ]]; then
        if grep -q "add_subdirectory.*tests" "$comp_path/CMakeLists.txt" 2> /dev/null; then
            cmake_has_tests=true
        fi
    fi

    # Determine status
    local status="missing"
    local details=""

    if [[ $has_tests == true && $has_cmake == true && $has_test_files == true && $cmake_has_tests == true ]]; then
        status="complete"
        details="$test_count test files"
    elif [[ $has_tests == true ]]; then
        status="partial"
        details="tests dir exists"
        if [[ $has_cmake == false ]]; then
            details="$details, no CMakeLists.txt"
        fi
        if [[ $has_test_files == false ]]; then
            details="$details, no test files"
        fi
        if [[ $cmake_has_tests == false ]]; then
            details="$details, not integrated"
        fi
    fi

    # Skip if only showing missing components
    if [[ $MISSING_ONLY == true && $status != "missing" ]]; then
        return
    fi

    # Print component status
    case "$status" in
    complete)
        print_component "$comp_name ($comp_type) - $details"
        ;;
    partial)
        print_partial "$comp_name ($comp_type) - $details"
        ;;
    missing)
        print_missing "$comp_name ($comp_type) - no test infrastructure"
        ;;
    esac

    # Show detailed analysis if requested
    if [[ $DETAILED == true ]]; then
        echo "    Path: $comp_path"
        echo "    Type: $comp_type"
        echo "    Tests directory: $([ "$has_tests" == true ] && echo "✓" || echo "✗")"
        echo "    Tests CMakeLists.txt: $([ "$has_cmake" == true ] && echo "✓" || echo "✗")"
        echo "    Test files: $([ "$has_test_files" == true ] && echo "✓ ($test_count)" || echo "✗")"
        echo "    Integrated: $([ "$cmake_has_tests" == true ] && echo "✓" || echo "✗")"
        echo ""
    fi

    # Update counters
    case "$status" in
    complete)
        COMPLETE_COUNT=$((COMPLETE_COUNT + 1))
        case "$comp_type" in
        util) UTIL_COMPLETE=$((UTIL_COMPLETE + 1)) ;;
        critical) CRITICAL_COMPLETE=$((CRITICAL_COMPLETE + 1)) ;;
        platform) PLATFORM_COMPLETE=$((PLATFORM_COMPLETE + 1)) ;;
        extension) EXTENSION_COMPLETE=$((EXTENSION_COMPLETE + 1)) ;;
        other) OTHER_COMPLETE=$((OTHER_COMPLETE + 1)) ;;
        esac
        ;;
    partial)
        PARTIAL_COUNT=$((PARTIAL_COUNT + 1))
        case "$comp_type" in
        util) UTIL_PARTIAL=$((UTIL_PARTIAL + 1)) ;;
        critical) CRITICAL_PARTIAL=$((CRITICAL_PARTIAL + 1)) ;;
        platform) PLATFORM_PARTIAL=$((PLATFORM_PARTIAL + 1)) ;;
        extension) EXTENSION_PARTIAL=$((EXTENSION_PARTIAL + 1)) ;;
        other) OTHER_PARTIAL=$((OTHER_PARTIAL + 1)) ;;
        esac
        ;;
    missing)
        MISSING_COUNT=$((MISSING_COUNT + 1))
        case "$comp_type" in
        util) UTIL_MISSING=$((UTIL_MISSING + 1)) ;;
        critical) CRITICAL_MISSING=$((CRITICAL_MISSING + 1)) ;;
        platform) PLATFORM_MISSING=$((PLATFORM_MISSING + 1)) ;;
        extension) EXTENSION_MISSING=$((EXTENSION_MISSING + 1)) ;;
        other) OTHER_MISSING=$((OTHER_MISSING + 1)) ;;
        esac
        ;;
    esac

    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    case "$comp_type" in
    util) UTIL_TOTAL=$((UTIL_TOTAL + 1)) ;;
    critical) CRITICAL_TOTAL=$((CRITICAL_TOTAL + 1)) ;;
    platform) PLATFORM_TOTAL=$((PLATFORM_TOTAL + 1)) ;;
    extension) EXTENSION_TOTAL=$((EXTENSION_TOTAL + 1)) ;;
    other) OTHER_TOTAL=$((OTHER_TOTAL + 1)) ;;
    esac
}

# Initialize counters
TOTAL_COUNT=0
COMPLETE_COUNT=0
PARTIAL_COUNT=0
MISSING_COUNT=0

UTIL_TOTAL=0
UTIL_COMPLETE=0
UTIL_PARTIAL=0
UTIL_MISSING=0

CRITICAL_TOTAL=0
CRITICAL_COMPLETE=0
CRITICAL_PARTIAL=0
CRITICAL_MISSING=0

PLATFORM_TOTAL=0
PLATFORM_COMPLETE=0
PLATFORM_PARTIAL=0
PLATFORM_MISSING=0

EXTENSION_TOTAL=0
EXTENSION_COMPLETE=0
EXTENSION_PARTIAL=0
EXTENSION_MISSING=0

OTHER_TOTAL=0
OTHER_COMPLETE=0
OTHER_PARTIAL=0
OTHER_MISSING=0

# Find all component directories
print_header "Azure Device Update Agent - Test Infrastructure Analysis"

# Find all component directories
print_header "Azure Device Update Agent - Test Infrastructure Analysis"

# Initialize counters
TOTAL_COUNT=0
COMPLETE_COUNT=0
PARTIAL_COUNT=0
MISSING_COUNT=0

# Analyze each component
echo "Scanning for components..."
components_found=0

# Find all directories with CMakeLists.txt (but not test directories)
while IFS= read -r -d '' comp_path; do
    if [[ ! $comp_path =~ /tests/ && -f $comp_path ]]; then
        comp_dir=$(dirname "$comp_path")
        analyze_component "$comp_dir"
        components_found=$((components_found + 1))
    fi
done < <(find src -name "CMakeLists.txt" -print0)

if [[ $components_found -eq 0 ]]; then
    echo "No components found. Please run from the ADU repository root."
    exit 1
fi

# Print summary
echo ""
print_header "Summary"

if [[ $MISSING_ONLY == false ]]; then
    print_section "Overall Status"
    echo "Total Components: $TOTAL_COUNT"
    echo -e "Complete Test Infrastructure: ${GREEN}$COMPLETE_COUNT${NC} ($((COMPLETE_COUNT * 100 / TOTAL_COUNT))%)"
    echo -e "Partial Test Infrastructure: ${YELLOW}$PARTIAL_COUNT${NC} ($((PARTIAL_COUNT * 100 / TOTAL_COUNT))%)"
    echo -e "Missing Test Infrastructure: ${RED}$MISSING_COUNT${NC} ($((MISSING_COUNT * 100 / TOTAL_COUNT))%)"
    echo ""

    print_section "By Component Type"
    if [[ $UTIL_TOTAL -gt 0 ]]; then
        echo -e "Utility Libraries: ${GREEN}$UTIL_COMPLETE${NC}/${YELLOW}$UTIL_PARTIAL${NC}/${RED}$UTIL_MISSING${NC} (Complete/Partial/Missing) of $UTIL_TOTAL"
    fi
    if [[ $CRITICAL_TOTAL -gt 0 ]]; then
        echo -e "Critical Components: ${GREEN}$CRITICAL_COMPLETE${NC}/${YELLOW}$CRITICAL_PARTIAL${NC}/${RED}$CRITICAL_MISSING${NC} (Complete/Partial/Missing) of $CRITICAL_TOTAL"
    fi
    if [[ $PLATFORM_TOTAL -gt 0 ]]; then
        echo -e "Platform Components: ${GREEN}$PLATFORM_COMPLETE${NC}/${YELLOW}$PLATFORM_PARTIAL${NC}/${RED}$PLATFORM_MISSING${NC} (Complete/Partial/Missing) of $PLATFORM_TOTAL"
    fi
    if [[ $EXTENSION_TOTAL -gt 0 ]]; then
        echo -e "Extension Components: ${GREEN}$EXTENSION_COMPLETE${NC}/${YELLOW}$EXTENSION_PARTIAL${NC}/${RED}$EXTENSION_MISSING${NC} (Complete/Partial/Missing) of $EXTENSION_TOTAL"
    fi
    if [[ $OTHER_TOTAL -gt 0 ]]; then
        echo -e "Other Components: ${GREEN}$OTHER_COMPLETE${NC}/${YELLOW}$OTHER_PARTIAL${NC}/${RED}$OTHER_MISSING${NC} (Complete/Partial/Missing) of $OTHER_TOTAL"
    fi
fi

# Print recommendations
if [[ $MISSING_COUNT -gt 0 || $PARTIAL_COUNT -gt 0 ]]; then
    echo ""
    print_section "Recommendations"

    if [[ $CRITICAL_MISSING -gt 0 || $CRITICAL_PARTIAL -gt 0 ]]; then
        echo -e "${RED}HIGH PRIORITY:${NC} Complete test infrastructure for critical components"
    fi

    if [[ $UTIL_MISSING -gt 0 || $UTIL_PARTIAL -gt 0 ]]; then
        echo -e "${YELLOW}MEDIUM PRIORITY:${NC} Complete test infrastructure for utility libraries"
    fi

    echo ""
    echo "To bootstrap test infrastructure for a component, use:"
    echo "  ./scripts/create_test_infrastructure.sh <component_path> [component_type]"
    echo ""
    echo "Examples:"
    if [[ $UTIL_MISSING -gt 0 ]]; then
        echo "  ./scripts/create_test_infrastructure.sh src/utils/string_utils util"
    fi
    if [[ $CRITICAL_MISSING -gt 0 ]]; then
        echo "  ./scripts/create_test_infrastructure.sh src/agent/api critical"
    fi
    if [[ $PLATFORM_MISSING -gt 0 ]]; then
        echo "  ./scripts/create_test_infrastructure.sh src/platform_layers/linux platform"
    fi
    if [[ $EXTENSION_MISSING -gt 0 ]]; then
        echo "  ./scripts/create_test_infrastructure.sh src/extensions/step_handlers/script_handler extension"
    fi
fi

echo ""
print_section "Legend"
echo -e "${GREEN}✓${NC} Complete test infrastructure (tests dir, CMakeLists.txt, test files, integration)"
echo -e "${YELLOW}~${NC} Partial test infrastructure (missing some components)"
echo -e "${RED}✗${NC} Missing test infrastructure"

if [[ $MISSING_ONLY == false ]]; then
    echo ""
    print_section "Usage"
    echo "Run with --missing-only to see only components that need work"
    echo "Run with --detailed for detailed analysis of each component"
    echo "Run with --component-type to filter by component type (util|critical|platform|extension)"
fi
