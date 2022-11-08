#!/bin/sh

# Note: This script is run by Dockerfile RUN directive when starting the container (see Dockerfile.template)

set -e

# from preinst
addgroup --system adu
adduser --system adu --ingroup adu --no-create-home --shell /bin/false

# from postinst
adu_conf_dir=/etc/adu
adu_conf_file=du-config.json
adu_diagnostics_conf_file=du-diagnostics-config.json
adu_log_dir=/var/log/adu
adu_data_dir=/var/lib/adu
adu_downloads_dir="$adu_data_dir/downloads"
adu_shell_dir=/usr/lib/adu
adu_shell_file=adu-shell

adu_extensions_dir="$adu_data_dir/extensions"
adu_extensions_sources_dir="$adu_extensions_dir/sources"

adu_user=adu
adu_group=adu

adu_home_dir=/home/adu

mkdir -p "$adu_conf_dir"
chown "$adu_user:$adu_group" "$adu_conf_dir"
chmod u=rwx,g=rx,o=rx "$adu_conf_dir"
chmod u=rw,g=r,o=r "$adu_conf_dir/$adu_conf_file"

chown "$adu_user:$adu_group" "$adu_conf_dir/$adu_diagnostics_conf_file"
chmod u=r,g=r,o=r "$adu_conf_dir/$adu_diagnostics_conf_file"

if [ ! -d "$adu_home_dir" ]; then
    mkdir -p "$adu_home_dir"
fi

chown "$adu_user:$adu_group" "$adu_home_dir"
chmod u=rwx,g=rx "$adu_home_dir"

# Create log dir
if [ ! -d "$adu_log_dir" ]; then
    mkdir -p "$adu_log_dir"
fi

# Ensure the right owners and permissions
chown "$adu_user:$adu_group" "$adu_log_dir"
chmod u=rwx,g=rx "$adu_log_dir"

# Create data dir
echo "Create data dir..."
if [ ! -d "$adu_data_dir" ]; then
    mkdir -p "$adu_data_dir"
fi

# Ensure the right owners and permissions
chown "$adu_user:$adu_group" "$adu_data_dir"
chmod u=rwx,g=rwx,o= "$adu_data_dir"
ls -ld "$adu_data_dir"

# Create downloads dir
if [ ! -d "$adu_downloads_dir" ]; then
    mkdir -p "$adu_downloads_dir"
fi

# Ensure the right owners and permissions
chown "$adu_user:$adu_group" "$adu_downloads_dir"
chmod u=rwx,g=rwx,o= "$adu_downloads_dir"
ls -ld "$adu_downloads_dir"

# Create extensions dir
if [ ! -d "$adu_extensions_dir" ]; then
    mkdir -p "$adu_extensions_dir"
fi

# Ensure the right owners and permissions
chown "$adu_user:$adu_group" "$adu_extensions_dir"
chmod u=rwx,g=rwx,o= "$adu_extensions_dir"

# Create extensions sources dir
if [ ! -d "$adu_extensions_sources_dir" ]; then
    mkdir -p "$adu_extensions_sources_dir"
fi

# Ensure the right owners and permissions
chown "$adu_user:$adu_group" "$adu_extensions_sources_dir"
chmod u=rwx,g=rwx,o= "$adu_extensions_sources_dir"

# Set deviceupdate-agent owner and permission
chown "root:$adu_group" "$adu_shell_dir/$adu_shell_file"
chmod u=rxs "$adu_shell_dir/$adu_shell_file"

#
# misc for healthcheck
#

addgroup --system 'do'
adduser --system 'do' --ingroup 'do' --no-create-home --shell /bin/false
usermod -aG 'adu' 'do'
usermod -aG 'do' 'adu'
chown root:root /usr/bin/AducIotAgent
chown adu:adu /etc/adu/du-config.json
