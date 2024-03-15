#!/bin/bash

# Get list of installed snaps
installed_snaps=$(curl -sX GET --unix-socket /run/snapd.socket http://localhost/v2/snaps | jq -r '.result[].name')

# Get list of snaps available in the store
available_snaps=$(curl -sX GET --unix-socket /run/snapd.socket http://localhost/v2/find?select=common-id,name | jq -r '.result[] | .name + " : " + (.["download-size"] / 1000000 | tostring) + " MB"'

# Print out both lists
echo "Installed snaps"
echo "==============="
echo $installed_snaps
echo
echo "Available snaps"
echo "==============="
echo $available_snaps
echo

echo "Snaps not installed"
echo "==================="
# Print out names of snaps available in the store but not installed
for snap in $available_snaps; do
  if [[ ! $installed_snaps =~ $snap ]]; then
    echo "$snap is available in the store but not installed"
  fi
done
