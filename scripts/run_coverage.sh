#!/usr/bin/env bash
set -euo pipefail

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }
error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

# This script mainly does 3 things:
# 1) Build the code with coverage instrumentation.
# 2) Run unit tests to produce execution data (.gcda).
# 3) Generate and publish a Cobertura coverage report.

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
root_dir="$script_dir/.."

out_dir="$root_dir/out"
build_type="Debug"
do_clean=true
report_only=false

# Any unrecognized args are passed through to build.sh.
declare -a build_sh_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
    -o | --out-dir)
        shift
        if [[ $# -eq 0 || $1 == -* ]]; then
            error "--out-dir requires a value"
            exit 1
        fi
        out_dir="$1"
        shift
        ;;
    -t | --type)
        shift
        if [[ $# -eq 0 || $1 == -* ]]; then
            error "--type requires a value"
            exit 1
        fi
        build_type="$1"
        shift
        ;;
    --no-clean)
        do_clean=false
        shift
        ;;
    --report-only)
        report_only=true
        shift
        ;;
    --coverage)
        error "--coverage is not supported by run_coverage.sh. Use ./scripts/build.sh --coverage"
        exit 1
        ;;
    -h | --help)
        cat << 'EOS'
Usage: run_coverage.sh [options] [-- <extra build.sh args...>]

Builds with coverage instrumentation, runs unit tests, and generates a gcovr
Cobertura XML report for Azure DevOps CI integration.

Options:
  -o, --out-dir <dir>     Build output directory (default: ../out)
  -t, --type <type>       Build type passed to build.sh (default: Debug)
  --no-clean              Do not run build.sh --clean
  --report-only           Skip build+ctest; generate report from existing .gcda data
  -h, --help              Show this help

Examples:
  ./scripts/run_coverage.sh
  ./scripts/run_coverage.sh --report-only
EOS
        exit 0
        ;;
    --)
        shift
        for arg in "$@"; do
            if [[ $arg == "--coverage" ]]; then
                error "--coverage is not supported by run_coverage.sh. Use ./scripts/build.sh --coverage"
                exit 1
            fi
        done
        build_sh_args+=("$@")
        break
        ;;
    *)
        build_sh_args+=("$1")
        shift
        ;;
    esac
done

# Coverage instrumentation flags used by the build step.
export CFLAGS="--coverage -O0 -g"
export CXXFLAGS="--coverage -O0 -g"
export LDFLAGS="--coverage"

# 1) Build (skipped in --report-only mode).
if [[ $report_only != "true" ]]; then
    # Build the unit-test target with instrumentation enabled.
    # Guard against recursive delegation if --coverage is forwarded by mistake.
    declare -a filtered_build_sh_args=()
    for arg in "${build_sh_args[@]}"; do
        if [[ $arg == "--coverage" ]]; then
            warn "Ignoring nested --coverage forwarded to build.sh to avoid recursion."
            continue
        fi
        filtered_build_sh_args+=("$arg")
    done

    declare -a invoke_build_sh_args=()
    if [[ $do_clean == "true" ]]; then
        invoke_build_sh_args+=("--clean")
    fi
    invoke_build_sh_args+=("-u" "-t" "$build_type" "-o" "$out_dir")
    invoke_build_sh_args+=("${filtered_build_sh_args[@]}")

    pushd "$root_dir" > /dev/null
    ./scripts/build.sh "${invoke_build_sh_args[@]}"
    popd > /dev/null
fi

# 2) Run unit tests (or validate existing .gcda data in --report-only mode).
if ! command -v ctest > /dev/null 2>&1; then
    error "ctest not found in PATH"
    exit 1
fi

if ! command -v gcovr > /dev/null 2>&1; then
    error "gcovr not found in PATH"
    error "Install with: pip install gcovr"
    exit 1
fi

pushd "$out_dir" > /dev/null
if [[ $report_only != "true" ]]; then
    ctest --output-on-failure
else
    if ! find . -name '*.gcda' -print -quit | grep -q .; then
        error "No .gcda files found under '$out_dir'. Run instrumented tests first or omit --report-only."
        exit 3
    fi
fi

coverage_report_dir="$out_dir/coverage"
coverage_xml_path="$coverage_report_dir/Cobertura.xml"
mkdir -p "$coverage_report_dir"

# 3) Generate Cobertura report from collected coverage data.
gcovr \
    --root "$root_dir" \
    --object-directory "$out_dir" \
    --filter ".*/src/.*" \
    --exclude ".*/tests?/.*" \
    --exclude ".*/Testing/.*" \
    --xml-pretty \
    --output "$coverage_xml_path" \
    --print-summary

if [[ ! -s $coverage_xml_path ]]; then
    warn "Cobertura report not generated: $coverage_xml_path"
    exit 2
fi

echo "Coverage report (Cobertura XML): $coverage_xml_path"

# Publish to Azure DevOps when running in pipeline context.
if [[ ${TF_BUILD:-} == "True" || ${TF_BUILD:-} == "true" ]]; then
    echo "##vso[codecoverage.publish codecoveragetool=Cobertura;summaryfile=$coverage_xml_path;reportdirectory=$coverage_report_dir;]"
fi
popd > /dev/null
