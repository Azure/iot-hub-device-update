#!/bin/bash

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi
 
# Use sudo if user is not root
SUDO=""
if  [ "$(id -u)" != "0" ]; then
    SODO="sudo"
fi
 
warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }
error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }
 
print_help() {
    echo ""
    echo "Usage: setup-local-package-repo.sh [options...]"
    echo ""
    echo "  -d <packages-full-path>       The fully qualified path to directory that contains your packages."
    echo ""
}
 
# Setup defaults
private_packages_dir=/tmp/my_private_packages
local_packages_dir=/var/lib/local_packages
private_packages_list=/etc/apt/sources.list.d/my-private-packages.list
 
create_repo() {
    echo '# Instaling dpkg-dev...'
    apt-get -y install dpkg-dev
 
    echo "# Create local package repos ($local_packages_dir)..."
    mkdir -p $local_packages_dir
 
    echo "# Copying private package(s) from '$private_pakcages_dir' to '$local_packages_dir'..."
    cp -f $private_packages_dir/*.deb $local_packages_dir
    ls -l -R $local_packages_dir
 
    echo "# Updating $private_packages_list ..."
    echo "deb [trusted=yes] file:$local_packages_dir ./" > $private_packages_list
    cat $private_packages_list
 
    echo '# Creating test packages catalogs...'
    cd $local_packages_dir
    dpkg-scanpackages . /dev/null | gzip -9c > Packages.gz
    
    echo '# Updating APT sources cache...'
    apt-get update
 
    echo '#'
    echo '# Done! See list of available adu-agent packages below... '
    echo '#'
    apt-cache policy deviceupdate-agent ms-doclient-lite ms-docsdk
}
 
# Check if no options were specified.
if [[ $1 == "" ]]; then
    error "Must specified at least one options."
    print_help
    $ret 1
fi
 
# Parse cmd options
while [[ $1 != "" ]]; do
    case $1 in
    -h | --help)
        print_help
        $ret 0
        ;;
    -d)
        shift
        private_packages_dir=$1
        ;;
    esac
    shift
done
 
if [[ $private_packages_dir != "" ]]; then
    $SUDO create_repo
fi
 
$ret 0