#!/bin/bash

BASE_SOURCE_DIR=$(pwd)
TEMPLATES_SOURCE_DIR=$BASE_SOURCE_DIR/templates

DEB_FILE='deviceupdate-agent_0.8.0_public_preview_amd64.deb'
DU_AGENT_URL="https://github.com/Azure/iot-hub-device-update/releases/download/0.8.0/$DEB_FILE"
DEBFILE_EXPECTED_DIGEST="GDpgPJ7jcM/ngY0EGCT/7OfU7QN1uvEAH6Dn6YOPds0="

CLEAN=0
DEB_PKG_PATH=''
SYSTEM_DEPS="
    ar
    docker
    sed
    sha256sum
    tar
    wget
"

ADD_DEBUGGING_PACKAGES=0
DBG_APT_PACKAGES="binutils gdb git iputils-ping iproute2 net-tools"

# Work directories
BASE_WORK_DIR=/tmp/adu/build_docker_img
APP_WORK_DIR=$BASE_WORK_DIR/app
APP_ROOT_DIR=$APP_WORK_DIR/root
APP_BIN_DIR=$APP_ROOT_DIR/usr/bin
APP_DEPS_LIB_DIR=$APP_ROOT_DIR/usr/lib
APP_LIB_DIR=$APP_DEPS_LIB_DIR/adu
APP_EXT_BASE_DIR=$APP_ROOT_DIR/var/lib/adu/extensions
APP_EXT_DIR=$APP_EXT_BASE_DIR/sources
APP_ETC_DIR=$APP_ROOT_DIR/etc/adu
TMP_WORK_DIR=$BASE_WORK_DIR/tmp
TMP_DATA_DIR=$TMP_WORK_DIR/data
TMP_CTRL_DIR=$TMP_WORK_DIR/control

USR_BIN_SRC=$TMP_DATA_DIR/usr/bin
USR_LIB_SRC=$TMP_DATA_DIR/usr/lib/adu
EXT_SRC=$TMP_DATA_DIR/var/lib/adu/extensions/sources

# commands
mkdir_cmd="mkdir -p"
wget_cmd="wget -q"
extract_debian_archive_cmd="ar x"
extract_tarball_cmd="tar -xzvf"
untar_cmd="tar -xvf"
create_tarball_cmd="tar -czvf"

#container image
CONTAINER_IMAGE_VERSION="1.0"
CONTAINER_IMAGE_DESCRIPTION="Azure IotHub DeviceUpdate Agent image"
CONTAINER_IMAGE_BASE_IMAGE="mcr.microsoft.com/mirror/docker/library/ubuntu:18.04" # MCR golden image
CONTAINER_IMAGE_TAG="duagentaptcurlubuntu18.04stable:$CONTAINER_IMAGE_VERSION"
CONTAINER_IMAGE_DOCKER_CMD='exec /usr/bin/AducIotAgent -l0 -e > /var/log/adu/DUAgent.log 2>&1' # Use exec to honor OS signals and 'docker stop'

# du-config vars
DUCONFIG_COMPAT_PROPERTY_NAMES="manufacturer,model" # used for update compatibility check and device groups
DUCONFIG_DEVICEINFO_MANUFACTURER="contoso"
DUCONFIG_DEVICEINFO_MODEL="toaster"
DUCONFIG_AGENT_NAME="dockerAptCurlAgent"
DUCONFIG_CONNECTION_STRING=

# The following are used for update compatibility
DUCONFIG_DEVICEPROPERTIES_MANUFACTURER="$DUCONFIG_DEVICEINFO_MANUFACTURER"
DUCONFIG_DEVICEPROPERTIES_MODEL="$DUCONFIG_DEVICEINFO_MODEL"

#
# Exit codes
#
EXIT_CODE_BAD_INVALID_CONNECTION_STRING_PATH=1
EXIT_CODE_UNMET_SYSTEM_DEPS=2
EXIT_CODE_BAD_DIGEST=3

usage() {
    cat << EOF
Usage: ./scripts/build_container.sh -c <path/to/file/with/connection string>

  Builds a docker container image with DU core agent, extensions, and dependencies.

    General options
      -h | --help                           Shows this help

    DU Config options
      -c | --connection-string  (required)  The path to file that contains connection string.
      -d | --deb-package-path   (optional)  The path to DU agent .deb to use instead of downloading release asset.
      -M | --manufacturer       (optional)  The value of manufacturer in du-config.json deviceProperties.
      -m | --model              (optional)  The value of model in du-config.json deviceProperties.

    Container options
      -i | --imgver             (optional)  The container image version.
      -b | --base               (optional)  The base container image to use.
      -t | --tag                (optional)  The container image tag.
      -g | --add-debugging      (optional)  Adds gdb, binutils, net-tools, etc. for debugging purposes.

EOF
}

inline_expand_template_parameters() {
    params_to_replace="$1"
    output_file_path="$2"

    for var in $params_to_replace; do
        pattern="%%${var}%%"
        param_val=${!var}
        param_val=${param_val/\./\\\.} # escape periods
        param_val=${param_val/\&/\\\&} # escape ampersands

        sed -i "s|${pattern}|${param_val}|g" "$output_file_path"
    done
}

create_update_content_handler_extension_registration() {
    handlerid=$1
    so_name=$2
    local libname=${handlerid/\//_}
    libname=${libname/:/_}

    # SC2034 is irrelevant because base64digest is implicitly used by inline_expand_template_parameters via parameters_to_expand
    # shellcheck disable=SC2034
    base64digest=$(openssl dgst -binary "$EXT_SRC/${so_name}.so" | openssl base64)

    $mkdir_cmd "$APP_EXT_BASE_DIR/update_extensions/step_handlers/$libname"
    local target_filepath="$APP_EXT_BASE_DIR/update_content_handlers/${libname}/content_handler.json"
    cp "$TEMPLATES_SOURCE_DIR/content_handler.template.json" "$target_filepath"

    parameters_to_expand="
        so_name
        base64digest
        handlerid
    "
    inline_expand_template_parameters "$parameters_to_expand" "$target_filepath"
}

create_content_downloader_registration() {
    so_name="$1"

    $mkdir_cmd "$APP_EXT_BASE_DIR/content_downloader"
    local target_filepath="$APP_EXT_BASE_DIR/content_downloader/extension.json"
    cp "$TEMPLATES_SOURCE_DIR/content_downloader.extension.template.json" "$target_filepath"

    # SC2034 is irrelevant because base64digest is implicitly used by inline_expand_template_parameters via parameters_to_expand
    # shellcheck disable=SC2034
    base64digest=$(openssl dgst -binary "$EXT_SRC/${so_name}.so" | openssl base64)

    parameters_to_expand="
        so_name
        base64digest
    "
    inline_expand_template_parameters "$parameters_to_expand" "$target_filepath"
}

copy_deps_binaries() {
    local do_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu1804_x64-packages.tar"

    cd $TMP_WORK_DIR || exit
    $wget_cmd $do_url
    $untar_cmd ./ubuntu1804_x64-packages.tar
    mkdir libdo
    cp libdeliveryoptimization_0.6.0_amd64.deb libdo
    cd libdo || exit
    $extract_debian_archive_cmd libdeliveryoptimization_0.6.0_amd64.deb
    mkdir data
    cp data.tar.gz data
    cd data || exit
    $extract_tarball_cmd data.tar.gz
    cp usr/lib/* $APP_DEPS_LIB_DIR/
}

create_app_tarball() {
    #
    # Copy app binaries
    #

    # Copy app agent core binaries
    cp $USR_BIN_SRC/AducIotAgent $APP_BIN_DIR
    cp $USR_LIB_SRC/adu-shell $APP_LIB_DIR

    # Copy update content handler plugin modules
    supported_extensions="
        apt
        steps
        script
    "
    for ext in $supported_extensions; do
        cp "$EXT_SRC/libmicrosoft_${ext}_1.so" $APP_EXT_DIR
    done

    # Copy content downloader plugin modules
    cp $EXT_SRC/libcurl_content_downloader.so $APP_EXT_DIR

    # Create app plugin modules registrations
    create_update_content_handler_extension_registration 'microsoft/apt:1' 'libmicrosoft_apt_1'
    create_update_content_handler_extension_registration 'microsoft/script:1' 'libmicrosoft_script_1'
    create_update_content_handler_extension_registration 'microsoft/steps:1' 'libmicrosoft_steps_1'
    create_update_content_handler_extension_registration 'microsoft/update-manifest:4' 'libmicrosoft_steps_1' # update manifest handler currently uses steps lib
    create_content_downloader_registration 'libcurl_content_downloader'

    # Copy required assets for use by container image
    cp "$TEMPLATES_SOURCE_DIR/setup_container.sh" $APP_LIB_DIR/

    # Copy and expand template params for du-config.json
    cp "$TEMPLATES_SOURCE_DIR/du-config.template.json" $APP_ETC_DIR/du-config.json
    parameters_to_expand="
        DUCONFIG_COMPAT_PROPERTY_NAMES
        DUCONFIG_DEVICEINFO_MANUFACTURER
        DUCONFIG_DEVICEINFO_MODEL
        DUCONFIG_AGENT_NAME
        DUCONFIG_CONNECTION_STRING
        DUCONFIG_DEVICEPROPERTIES_MANUFACTURER
        DUCONFIG_DEVICEPROPERTIES_MODEL
    "
    inline_expand_template_parameters "$parameters_to_expand" "$APP_ETC_DIR/du-config.json"

    # Copy app du-diagnostics-config json
    cp "$TEMPLATES_SOURCE_DIR/du-diagnostics-config.template.json" $APP_ETC_DIR/du-diagnostics-config.json

    copy_deps_binaries

    $create_tarball_cmd $APP_WORK_DIR/app.tar.gz -C $APP_ROOT_DIR . > /dev/null 2>&1
}

#
# MAIN
#

# Parse cmd-line args
while [ "$1" != "" ]; do
    case $1 in
    -c | --connection-string)
        shift
        if [ ! -f "$1" ]; then
            echo "ERROR: $1 is not a valid path"
            exit $EXIT_CODE_BAD_INVALID_CONNECTION_STRING_PATH
        fi
        DUCONFIG_CONNECTION_STRING=$(cat "$1")
        ;;
    -C | --clean)
        CLEAN=1
        ;;
    -d | --deb-package-path)
        shift
        DEB_PKG_PATH="$1"
        ;;
    -g | --add-debugging)
        ADD_DEBUGGING_PACKAGES=1
        ;;
    -M | --manufacturer)
        shift
        DUCONFIG_DEVICEINFO_MANUFACTURER="$1"
        DUCONFIG_DEVICEPROPERTIES_MANUFACTURER="$1"
        ;;
    -m | --model)
        shift
        DUCONFIG_DEVICEINFO_MODEL="$1"
        DUCONFIG_DEVICEPROPERTIES_MODEL="$1"
        ;;

    -i | --imgver)
        shift
        CONTAINER_IMAGE_VERSION="$1"
        ;;
    -b | --base)
        shift
        CONTAINER_IMAGE_BASE_IMAGE="$1"
        ;;
    -t | --tag)
        shift
        CONTAINER_IMAGE_TAG="$1"
        ;;

    -h | --help)
        usage
        exit 0
        ;;
    *)
        usage
        exit 1
        ;;
    esac
    shift
done

if [ "$DUCONFIG_CONNECTION_STRING" = "" ]; then
    printf "\nERROR: -c | --connection-string is required.\n\n"
    usage
    exit 1
fi

cat <<- EOF
     Using the following:
     CONTAINER_IMAGE_VERSION: $CONTAINER_IMAGE_VERSION
     CONTAINER_IMAGE_DESCRIPTION: $CONTAINER_IMAGE_DESCRIPTION
     CONTAINER_IMAGE_BASE_IMAGE: $CONTAINER_IMAGE_BASE_IMAGE
     CONTAINER_IMAGE_TAG: $CONTAINER_IMAGE_TAG
     CONTAINER_IMAGE_DOCKER_CMD: $CONTAINER_IMAGE_DOCKER_CMD
     DUCONFIG_COMPAT_PROPERTY_NAMES: $DUCONFIG_COMPAT_PROPERTY_NAMES
     DUCONFIG_DEVICEINFO_MANUFACTURER: $DUCONFIG_DEVICEINFO_MANUFACTURER
     DUCONFIG_DEVICEINFO_MODEL: $DUCONFIG_DEVICEINFO_MODEL
     DUCONFIG_AGENT_NAME: $DUCONFIG_AGENT_NAME
     DUCONFIG_DEVICEPROPERTIES_MANUFACTURER: $DUCONFIG_DEVICEPROPERTIES_MANUFACTURER
     DUCONFIG_DEVICEPROPERTIES_MODEL: $DUCONFIG_DEVICEPROPERTIES_MODEL
EOF

#
# Ensure dependencies
#
DEP_FAILURE=0
for sysdep in $SYSTEM_DEPS; do
    which "$sysdep" > /dev/null 2>&1
    ret_val=$?
    if [ $ret_val -ne 0 ]; then
        DEP_FAILURE=1
        echo "Unmet system dependencies: $sysdep"
    fi
done

if [ $DEP_FAILURE -eq 1 ]; then
    echo "exiting due to unmet system dependencies"
    exit $EXIT_CODE_UNMET_SYSTEM_DEPS
fi

#
# Cleanup
#
if [ $CLEAN -eq 1 ]; then
    echo "Cleaning up '$APP_WORK_DIR' ..."
    [ -d $APP_WORK_DIR ] && rm -rf $APP_WORK_DIR

    echo "Cleaning up '$TMP_WORK_DIR' ..."
    [ -d $TMP_WORK_DIR ] && rm -rf $TMP_WORK_DIR
fi

#
# Create necessary dirs
#
DIRS_TO_CREATE="
    $APP_WORK_DIR
    $TMP_WORK_DIR
    $APP_BIN_DIR
    $APP_LIB_DIR
    $APP_EXT_DIR
    $APP_ETC_DIR
"
for d in $DIRS_TO_CREATE; do
    if [ ! -d "$d" ]; then
        $mkdir_cmd "$d"
    fi
done

#
# Download DU agent deb pkg
#

cd $TMP_WORK_DIR || exit
if [ "$DEB_PKG_PATH" = "" ]; then
    $wget_cmd $DU_AGENT_URL
    digest=$(openssl dgst -binary $DEB_FILE | openssl base64)
    if [ "$digest" != "$DEBFILE_EXPECTED_DIGEST" ]; then
        echo "bad hash of $DEB_FILE. Expected '$DEBFILE_EXPECTED_DIGEST' but got '$digest'"
        exit $EXIT_CODE_BAD_DIGEST
    fi
else
    DEB_FILE="$DEB_PKG_PATH"
fi

#
# Extract deb pkg
#
cd $TMP_WORK_DIR || exit
$extract_debian_archive_cmd "$DEB_FILE"

$mkdir_cmd $TMP_DATA_DIR
cp data.tar.gz $TMP_DATA_DIR
cd $TMP_DATA_DIR || exit
$extract_tarball_cmd data.tar.gz > /dev/null 2>&1

cd $TMP_WORK_DIR || exit
$mkdir_cmd $TMP_CTRL_DIR
cp control.tar.gz $TMP_CTRL_DIR
cd $TMP_CTRL_DIR || exit
$extract_tarball_cmd control.tar.gz > /dev/null 2>&1

create_app_tarball

#
# Create Dockerfile from template
#
cd $APP_WORK_DIR || exit
cp "${TEMPLATES_SOURCE_DIR}/Dockerfile.template" "$APP_WORK_DIR/Dockerfile"

# Replace template parameters with contents of variables with same name as template parameter.
APT_PACKAGES_FOR_DEBUGGING=''
if [ $ADD_DEBUGGING_PACKAGES -eq 1 ]; then
    export APT_PACKAGES_FOR_DEBUGGING="$DBG_APT_PACKAGES"
fi

params_to_replace="
    APT_PACKAGES_FOR_DEBUGGING
    CONTAINER_IMAGE_BASE_IMAGE
    CONTAINER_IMAGE_DESCRIPTION
    CONTAINER_IMAGE_DOCKER_CMD
    CONTAINER_IMAGE_VERSION
"
inline_expand_template_parameters "$params_to_replace" "$APP_WORK_DIR/Dockerfile"

#
# Build Docker image
#
cd $APP_WORK_DIR || exit
docker build --pull -f "Dockerfile" --rm -t "$CONTAINER_IMAGE_TAG" .
