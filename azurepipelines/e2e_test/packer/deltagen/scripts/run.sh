#!/bin/sh

#
# To be run from docker host after running:
# packer build .
#

usage() {
    cat <<- EOS
Usage: $(basename "$0") <src .swu URL> <tgt .swu URL> <output dir for delta file>
  e.g. $(basename "$0") https://example.com/source.swu https://example.com/target.swu /path/to/host/outdir/for/delta/
EOS

    exit 1
}

test "$#" -eq 3 || usage

docker_image=dudeltagen:latest

SRC_SWU_URL="$1"
TGT_SWU_URL="$2"
HOST_DELTA_OUTPUT_DIR="$3"

rm -rf /tmp/dockercp
mkdir -p /tmp/dockercp || exit 1

echo -e "\nDownloading Source .swu at $SRC_SWU_URL ..."
wget -O "$HOST_DELTA_OUTPUT_DIR/src.swu" "$SRC_SWU_URL" || exit 1

echo -e "\nDownloading Target .swu at $TGT_SWU_URL ..."
wget -O "$HOST_DELTA_OUTPUT_DIR/tgt.swu" "$TGT_SWU_URL" || exit 1

echo -e "\nRunning container with mount /deltaoutput bound to host's $HOST_DELTA_OUTPUT_DIR ..."
docker run -it --rm --name deltagen \
    -v "$HOST_DELTA_OUTPUT_DIR:/deltaoutput" \
    ${docker_image} \
    || exit 1

echo -e "\nAll done!"
