#!/bin/sh

unzip /tmp/linux64-diffgen-tool.zip

base_dir="linux64-diffgen-tool"

files="
 zstd_compress_file
 working_folder_manager.py
 sign_tool.py
 recompress_tool.py
 recompress_and_sign_tool.py
 helpers.py
 dumpextfs
 dumpdiff
 compress_files.py
 bspatch
 bsdiff
 applydiff
 DiffGenTool
 "
for x in $files; do
    chmod 755 "${base_dir}/$x"
done

cp "${base_dir}/libadudiffapi.so" /usr/local/lib/
ldconfig -v /usr/local/lib/libadudiffapi.so
