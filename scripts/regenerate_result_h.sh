#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Script to regenerate and format the result.h file from result_codes.json
# This script can be used independently of the full build process

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

# Define paths
ERROR_CODE_JSON_PATH="${ROOT_DIR}/scripts/error_code_generator_defs/result_codes.json"
ERROR_CODE_GENERATOR_SCRIPT="${ROOT_DIR}/scripts/error_code_generator_defs/error_code_defs_generator.py"
RESULT_H_PATH="${ROOT_DIR}/src/inc/aduc/result.h"

# Check if required files exist
if [[ ! -f $ERROR_CODE_JSON_PATH ]]; then
    echo "Error: result_codes.json not found at: $ERROR_CODE_JSON_PATH"
    exit 1
fi

if [[ ! -f $ERROR_CODE_GENERATOR_SCRIPT ]]; then
    echo "Error: error_code_defs_generator.py not found at: $ERROR_CODE_GENERATOR_SCRIPT"
    exit 1
fi

# Check if Python3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 is not installed or not in PATH"
    exit 1
fi

echo "Regenerating result.h file..."
echo "  Input JSON: $ERROR_CODE_JSON_PATH"
echo "  Output file: $RESULT_H_PATH"

# Generate the result.h file
if ! python3 "$ERROR_CODE_GENERATOR_SCRIPT" \
    --json-file-path "$ERROR_CODE_JSON_PATH" \
    --result-file-path "$RESULT_H_PATH"; then
    echo "Error: Failed to generate result.h file"
    exit 1
fi

echo "Successfully generated result.h file"

# Format the generated file with clang-format if available
if command -v clang-format &> /dev/null; then
    echo "Formatting result.h with clang-format..."
    if clang-format -style=file -i "$RESULT_H_PATH"; then
        echo "Successfully formatted result.h file"
    else
        echo "Warning: Failed to format result.h file with clang-format"
    fi
else
    echo "Warning: clang-format not found. Generated file will not be formatted."
fi

echo "Done. Result.h file has been regenerated at: $RESULT_H_PATH"
