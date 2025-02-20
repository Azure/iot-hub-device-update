#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Installs the deviceupdate-agent daemon and performs necessary setup/configuration.
# This script is meant to only be called from CMake as part of the install target.

# Any error should exit the script (sort of)
# See https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html#The-Set-Builtin
set -e

if [ "$#" -ne 5 ]; then
    echo "Expected 5 command line args, but got $#" >&2
    echo "Usage: install.sh configuration_dir configuration_file home_dir log_dir data_dir"
fi

adu_conf_dir=$1
adu_conf_file=$2
adu_home_dir=$3
adu_log_dir=$4
adu_data_dir=$5

# adu_user is the user that the ADU Agent daemon will run as.
# ADU Agent daemon needs to run as 'adu' to be able to perform high-privilege tasks via adu-shell.
adu_user=adu

# The adu_group is the group that gives partner users like DO user
# access to ADU resources like download sandbox folder.
adu_group=adu

# Note: DO user and group are created by deliveryoptimization-agent Debian package,
# which is one of the dependencies declared in deviceupdate-agent control file.
# We are assuming that both DO user and group currently exist at this point.

# The user that the DO Agent daemon runs as.
do_user='do'

# The sample du-config.json
sample_du_config=$(
    cat << END_OF_JSON
{
  "schemaVersion": "1.1",
  "aduShellTrustedUsers": [
    "adu",
    "do"
  ],
  "iotHubProtocol": "mqtt",
  "manufacturer": <Place your device info manufacturer here>,
  "model": <Place your device info model here>,
  "agents": [
    {
      "name": <Place your agent name here>,
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": <Place your Azure IoT device connection string here>
      },
      "manufacturer": <Place your device property manufacturer here>,
      "model": <Place your device property model here>
    }
  ]
}
END_OF_JSON
)

add_adu_user_and_group() {
    echo "Create the 'adu' group."
    if ! getent group "$adu_group" > /dev/null; then
        addgroup --system "$adu_group"
    else
        # For security purposes, it's extremely important that there's no existing 'adu' user and 'adu' group.
        # This ensures the account and group are setup by our package installation process only,
        # and no other actor can pre-create the user/group with nefarious intentions.
        echo "Error: 'adu' group already exists." >&2
        exit 1
    fi

    echo "Create the 'adu' user." # With no shell and no login, in 'adu' group.
    if ! getent passwd "$adu_user" > /dev/null; then
        adduser --system "$adu_user" --ingroup "$adu_group" --no-create-home --shell /bin/false
    else
        # For security purposes, it's extremely important that there's no existing 'adu' user and 'adu' group.
        # This to ensures the account and group are setup by our package installation process only,
        # and no other actor can pre-create the user/group with nefarious intentions.
        echo "Error: 'adu' user already exists." >&2
        exit 2
    fi

    echo "Add the 'adu' user to the 'syslog' group." # To allow ADU to write to /var/log folder
    if getent group "syslog" > /dev/null; then
        usermod -aG "syslog" "$adu_user"
    fi

    echo "Add the 'do' user to the 'adu' group." # To allow DO to write to ADU download sandbox.
    if getent passwd "$do_user" > /dev/null; then
        usermod -aG "$adu_group" "$do_user"
    fi
}

register_daemon() {
    systemctl daemon-reload
    systemctl enable deviceupdate-agent
}

setup_dirs_and_files() {
    # Note on linux permissions
    # u - The user owner of a file or a directory.
    # g - The group owner of a file or a directory. A group can contain multiple users.
    # o - Any other user that has access to the file or directory.
    # a - All. The combination of user owner, group ownder, and other users.
    # r - Read access.
    #   Read access on a file allows a user to open and read the contents of the file.
    #   Read access on a directory allows a user to list the contents of the directory. e.g with the ls command.
    # w - Write access.
    #   Write access on a file allows a user to modify the contents of a file.
    #   Write access on a directory allows a user to add, remove, rename, or move files in the directory.
    # x - Execute access.
    #   Execute access on a file allows a user to execute the contents of the file as a program.
    #   Execute access on a directory allows a user to enter that directory and possibly gain access to sub-directories. e.g. with the cd command.
    if id -u "$adu_user" > /dev/null 2>&1; then
        mkdir -p "$adu_conf_dir"

        # Generate the template configuration file
        if [ ! -f "$adu_conf_dir/${adu_conf_file}.template" ]; then
            echo "$sample_du_config" > "$adu_conf_dir/${adu_conf_file}.template"
            chown "$adu_user:$adu_group" "$adu_conf_dir/${adu_conf_file}.template"
            chmod u=r "$adu_conf_dir/${adu_conf_file}.template"
        fi

        # Create configuration file from template
        if [ ! -f "$adu_conf_dir/$adu_conf_file" ]; then
            cp -a "$adu_conf_dir/${adu_conf_file}.template" "$adu_conf_dir/$adu_conf_file"
            chmod u=rw "$adu_conf_dir/$adu_conf_file"
        fi

        # Create home dir
        if [ ! -d "$adu_home_dir" ]; then
            mkdir -p "$adu_home_dir"
            chown "$adu_user:$adu_group" "$adu_home_dir"
            chmod u=rwx,g=rx "$adu_home_dir"
        fi

        # Create log dir
        if [ ! -d "$adu_log_dir" ]; then
            mkdir -p "$adu_log_dir"
            chown "$adu_user:$adu_group" "$adu_log_dir"
            chmod u=rwx,g=rx "$adu_log_dir"
        fi

        # Create data dir
        if [ ! -d "$adu_data_dir" ]; then
            mkdir -p "$adu_data_dir"
            chown "$adu_user:$adu_group" "$adu_data_dir"
            chmod u=rwx,g=rx "$adu_data_dir"
        fi
    else
        echo "ERROR! $adu_user does not exist." >&2
        return 1
    fi
}

add_adu_user_and_group
setup_dirs_and_files
register_daemon

exit 0
