#!/bin/bash

#
# Runs the Delta Generation Tool using the private key at
# /deltaoutput/priv.pem to generate the delta.dat in the same directory.
#
# Expected input files:
# /deltaoutput/src.swu
# /deltaoutput/tgt.swu
#
# Outputs upon success:
# /deltaoutput/tgt-recompressed.swu - The new tgt.swu that has its ext-fs
#                                     disk img recompressed using the ZSTD
#                                     compression and an sw-description file
#                                     within the .swu CPIO archive that has
#                                     been resigned due to the changing hash
#                                     of the recompressed disk image.
#
# /deltaoutput/delta.dat            - The resultant delta file that has
#                                     microsoft delta recipe opcodes for the
#                                     libadudiffapi.so to apply the diff to
#                                     the src.swu that will be in the source
#                                     cache dir on the device receiving delta
#                                     updates.
#

base_dir="linux64-diffgen-tool"

io_dir="/deltaoutput"
delta_filepath="$io_dir/delta.dat"
log_dir="$io_dir/DiffGenTool_Logs"
work_dir="$io_dir/DiffGenTool_Work"
recompressed_target_swu_filepath="$io_dir/tgt-recompressed.swu"

mkdir -p "$log_dir" || exit 1
mkdir -p "$work_dir" || exit 1

cd "$base_dir" || exit 1

# This is the usage. The 3rd form is what is used below.
# Usage: DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder>
#        DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder> <recompressed target archive>
#        DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder> <recompressed target archive> "<signing command>"

./DiffGenTool \
    "$io_dir/src.swu" \
    "$io_dir/tgt.swu" \
    "$delta_filepath" \
    "$log_dir" \
    "$work_dir" \
    "$recompressed_target_swu_filepath" \
    'openssl dgst -sign /deltaoutput/priv.pem -keyform PEM -sha256 -out sw-description.sig -binary' \
    || exit 1
