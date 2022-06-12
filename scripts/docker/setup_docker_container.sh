#!/bin/bash
set -e

if [[ $# -ne 4 ]]; then
    echo "USAGE: $0 /path/to/dockerimage.tar.gz <CONNECTION STRING> <MANUFACTURER> <MODEL>"
    exit 1
fi

gunzip "$1"
dir_name=$(dirname "$1")
base_name=$(basename "$1" .gz)
docker load < "$dir_name/$base_name"
docker run -i -t --detach duagentaptcurlubuntu18.04stable:1.0 bash

tmp_container_id=$(docker ps | grep duagentaptcurl | cut -f1 -d' ')
# echo $tmp_container_id

docker cp "$tmp_container_id:/etc/adu/du-config.json" ./du-config.template.json
sed -i -e "s/CONNECTION_STRING_PLACEHOLDER/$2/g" du-config.template.json
sed -i -e "s/contoso/$3/g" du-config.template.json
sed -i -e "s/toaster/$4/g" du-config.template.json
docker cp du-config.template.json "$tmp_container_id:/etc/adu/du-config.json"

echo
echo "Attaching to docker container $tmp_container_id... Ctrl + P, Ctrl + Q to detach"
echo
docker attach "$tmp_container_id"
