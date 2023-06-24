#!/bin/bash

########################################################################
## DISCLAIMER: This script is for demonstration purposes only.
## It is not intended for production use.
########################################################################

echo "Combinding file '$1' and file '$2' into file '$3'"

# Delete file $3 if exists.
if [ -z "$3" ]; then rm -f "$3"; fi

# Combine files $1 and $2 into file $3
cat "$1" > $3
cat "$2" >> $3

# Exit with success code
exit 0
