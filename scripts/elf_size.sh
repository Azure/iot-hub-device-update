#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

PROG_NAME=$0

usage() {
    cat << EOF

Usage: ${PROG_NAME} [options] /path/to/elf/binary

  -p, --pattern          pattern of nm line output such as in symbol name.

  -s, --sum              Sum up the bytes of the matching ELF sections.

  -h, --help             Show this help message.


  Examples:

  ${PROG_NAME} -s -p zlog /usr/bin/AducIotAgent         # sum all contributions to ELF size for zlog library
  ${PROG_NAME} -s -p "azure::" /usr/bin/AducIotAgent    # sum all contributions from symbols that contain "azure::"

  ${PROG_NAME} /usr/bin/AducIotAgent | tail -n 5        # print top 5 contributors to binary size
EOF
}

SUM=0
PATTERN=0
ELF_FILEPATHS=()

while [[ $# -gt 0 ]]; do
    case $1 in
    -p | --pattern)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-p | --pattern requires value"
            exit 1
        fi
        PATTERN="$1"
        ;;

    -s | --sum)
        SUM=1
        ;;

    -h | --help)
        usage
        exit 0
        ;;

    --* | -*)
        echo bad option "$1"
        exit 1
        ;;

    *)
        ELF_FILEPATHS+=("$1")
        ;;
    esac
    shift
done

if [[ ${#ELF_FILEPATHS[@]} -eq 0 ]]; then
    echo no elf binary filepaths specified.
    exit 1
fi

IFS=
for elf_filpath in "${ELF_FILEPATHS[@]}"; do
    echo -e "\nProcessing $elf_filpath ...\n"

    result="$(nm -C --print-size --size-sort --radix=d "$elf_filpath")"

    if [[ -n $PATTERN ]]; then
        result="$(echo "$result" | grep "${PATTERN}")"
    fi

    if [[ $SUM == 1 ]]; then
        result="$(echo "$result" | awk '{print$2}' | awk '{s+=$1}END{print s}')"
        echo Total usage in bytes:
    fi

    echo -e "$result\n"
done
