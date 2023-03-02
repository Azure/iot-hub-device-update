#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# set -x

PROG_NAME=$0

usage() {
    cat << EOF

Usage: ${PROG_NAME} [options] /path/to/elf/binary

  -p, --pattern          pattern of nm line output such as in symbol name.

  -s, --sum              Sum up the bytes of the matching ELF sections.

  -b, --bss              Filter for BSS data section
  -t, --text             Filter for TEXT (code) section
  -w, --weak             Filter for weak symbols

  -h, --help             Show this help message.


  Examples:

  ${PROG_NAME} -s -p zlog /usr/bin/AducIotAgent         # sum all contributions to ELF size for zlog library
  ${PROG_NAME} -s -p "azure::" /usr/bin/AducIotAgent    # sum all contributions from symbols that contain "azure::"

  ${PROG_NAME} /usr/bin/AducIotAgent | tail -n 5        # print top 5 contributors to binary size

  ${PROG_NAME} --bss --text /usr/bin/AducIotAgent | tail -n 10   # print top ten BSS and TEXT
  ${PROG_NAME} --sum --weak -p "Azure::" /usr/bin/AducIotAgent   # sum footprint usage of only WEAK symbols that contain "Azure::"
EOF
}

PATTERN=
SUM=0
TEXT=0
BSS=0
WEAK=0
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

    -b | --bss)
        BSS=1
        ;;

    -t | --text)
        TEXT=1
        ;;

    -w | --weak)
        WEAK=1
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

    section_filter=""
    if [[ $TEXT == 1 ]]; then
        echo "Filtering for TEXT(code) section - " t " or " T " in nm output..."
        section_filter='t|T'
    fi

    if [[ $BSS == 1 ]]; then
        echo "Filtering for BSS data section - " b " or " B " in nm output..."
        if [[ -n $section_filter ]]; then
            section_filter="$section_filter|b|B"
        else
            section_filter='b|B'
        fi
    fi

    if [[ $WEAK == 1 ]]; then
        echo "Filtering for undefined symbols - " w " or " W " in nm output..."
        if [[ -n $section_filter ]]; then
            section_filter="$section_filter|w|W"
        else
            section_filter='w|W'
        fi
    fi

    if [[ $TEXT == 1 || $BSS == 1 || $WEAK == 1 ]]; then
        # match column 3 against the section_filter regex and print $0, the entire line.
        awk_expr="\$3~/($section_filter)/ {print \$0}"
        result="$(echo "$result" | awk "$awk_expr")"
    fi

    if [[ -n $PATTERN ]]; then
        echo Pattern: "$PATTERN"
        result="$(echo "$result" | grep "${PATTERN}")"
    fi

    if [[ $SUM == 1 ]]; then
        result="$(echo "$result" | awk '{print $2}' | awk '{s+=$1}END{print s}')"
        echo Total usage in bytes:
    fi

    echo -e "$result\n"
done
