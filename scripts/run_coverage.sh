#!/usr/bin/env bash
set -euo pipefail

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }
error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

# This script mainly does 2 things:
# 1) Run unit tests to produce execution data (.gcda).
# 2) Generate and publish a Cobertura coverage report.

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
root_dir="$script_dir/.."

out_dir="$root_dir/out"

exclude_patterns=(
    ".*/tests?/.*"
    ".*/Testing/.*"
    # These extension paths contain code examples, so they do not need coverage reporting.
    ".*/src/extensions/component_enumerators/.*"
    ".*/src/extensions/content_downloaders/deliveryoptimization_downloader/.*"
    ".*/src/extensions/step_handlers/simulator_handler/.*"
)

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
    -h | --help)
        cat << 'EOS'
Usage: run_coverage.sh [options]

Runs unit tests and generates a gcovr Cobertura XML report for Azure DevOps CI
integration.

Options:
  -o, --out-dir <dir>     Build output directory (default: ../out)
  -h, --help              Show this help

Examples:
  ./scripts/run_coverage.sh
EOS
        exit 0
        ;;
    *)
        error "Invalid argument: $1"
        exit 1
        ;;
    esac
done

# 1) Run unit tests to produce .gcda files.
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
ctest --output-on-failure

coverage_report_dir="$out_dir/coverage"
coverage_xml_path="$coverage_report_dir/Cobertura.xml"
mkdir -p "$coverage_report_dir"

# 2) Generate Cobertura report from collected coverage data.
gcovr_args=(
    --root "$root_dir"
    --object-directory "$out_dir"
    --filter ".*/src/.*"
)

for pattern in "${exclude_patterns[@]}"; do
    gcovr_args+=(--exclude "$pattern")
done

gcovr_args+=(
    --xml-pretty
    --output "$coverage_xml_path"
    --print-summary
)

gcovr "${gcovr_args[@]}"

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
