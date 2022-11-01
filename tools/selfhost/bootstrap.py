#!/usr/bin/env python3

import argparse
import json
import jsonschema
import os
import requests
import shutil
import subprocess
import sys

from jsonschema import validate
from urllib.parse import urlparse

Verbose = False

# where some useful self-hoster bugbash files will be copied
content_dir = '/tmp/deltaupdates'


def get_filepath_relative_to_script_dir(filename: str) -> str:
    """Builds a file path relative to the location where this script"""
    script_dir = os.path.dirname(sys.argv[0])
    return os.path.join(script_dir, filename)


def mkdirs_if_not_exist(dir_path: str) -> None:
    """makes the dir and its intermediates if it doesn't already exist."""
    if not os.path.exists(dir_path):
        os.makedirs(dir_path, mode=0o777, exist_ok=True)


def get_schema() -> str:
    """This function loads the boostrap config schema."""
    bootstrap_config_schema_path = get_filepath_relative_to_script_dir(
        filename='bootstrap_config_schema.json'
        )

    with open(bootstrap_config_schema_path, 'r') as file:
        schema = json.load(file)

    return schema


def validate_config(json_data):
    """Validates the json_data against the config json schema."""
    execute_api_schema = get_schema()

    try:
        validate(instance=json_data, schema=execute_api_schema)
    except jsonschema.exceptions.ValidationError as err:
        print(err)
        err = "Config JSON data is Invalid"
        return False, err

    err = None
    return True, err


def get_config_example() -> str:
    """Returns the contents of the config example file"""
    example_json_path = get_filepath_relative_to_script_dir(
        filename='bootstrap_config_example.json'
        )
    with open(example_json_path, 'r') as file:
        contents = json.load(file)
    return contents


def run_cmd(cmd: str) -> int:
    # ubuntu18.04 has python 3.6 so can't use capture_output=VERBOSE so set
    # stdout and stderr to PIPE instad
    ret = subprocess.run(cmd, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, shell=True
                         )
    ret_code = ret.returncode
    if ret_code != 0:
        print(f"Error: returncode {ret_code} for cmd '{cmd}'.{os.linesep}"
              f"Contents of <stderr>:{os.linesep}{ret.stderr.decode()}"
              )
    elif Verbose:
        print(ret.stdout.decode())
    return ret_code


def install_do(script_dir):
    """Installs delivery optimization components from the bundle"""

    print("Installing DeliveryOptimization packages ...")

    tmp_dir = os.path.join(script_dir, 'do_package')
    filename = 'ubuntu1804_x64-packages.tar'
    script_tar_file = os.path.join(script_dir, filename)

    ret_code = run_cmd(
        f"mkdir -p {tmp_dir} "
        f"&& cp {script_tar_file} {tmp_dir} "
        f"&& cd {tmp_dir} "
        f"&& tar -xvf ./{filename}"
        )
    if ret_code != 0:
        return ret_code

    # note: These are 0.8.3 .deb packages but in the tarball they still have
    # 0.6.0 in the name.
    do_agent_pkg = os.path.join(
        tmp_dir,
        'deliveryoptimization-agent_0.6.0_amd64.deb'
        )
    ret_code = run_cmd(f"apt-get -y install {do_agent_pkg}")
    if ret_code != 0:
        return ret_code

    libdo_pkg = os.path.join(
        tmp_dir,
        'libdeliveryoptimization_0.6.0_amd64.deb'
        )
    ret_code = run_cmd(f"apt-get -y install {libdo_pkg}")
    if ret_code != 0:
        return ret_code

    libdo_apt_plugin_pkg = os.path.join(
        tmp_dir,
        'deliveryoptimization-plugin-apt_0.4.0_amd64.deb'
        )
    ret_code = run_cmd(f"apt-get -y install {libdo_apt_plugin_pkg}")
    if ret_code != 0:
        return ret_code

    return 0


def install_du_agent(script_dir):
    """Installs device update agent component from the bundle"""

    print("Installing DU Agent ...")

    do_agent_pkg = os.path.join(script_dir, 'du_agent_pkg.deb')
    ret_code = run_cmd(f"apt-get -y install {do_agent_pkg}")
    if ret_code != 0:
        return ret_code

    return 0


def install_deltaupdate_swupdate_files(script_dir):
    """Installs swupdate-related binaries and configs from the bundle"""

    print("Installing DeltaUpdate feature swupdate-related files ...")

    tmp_dir = os.path.join(script_dir, 'delta_swupdate_files')
    filename = 'delta_swupdate_files.tar.gz'
    tarball_path = os.path.join(script_dir, filename)

    print(f"Unpacking {filename} to {tmp_dir} ...")

    ret_code = run_cmd(
        f"mkdir -p {tmp_dir} "
        f"&& cp {tarball_path} {tmp_dir} "
        f"&& cd {tmp_dir} "
        f"&& tar -xzvf {filename} "
        )
    if ret_code != 0:
        return ret_code

    print("Installing swupdate executuble and configs ...")

    ret_code = run_cmd(
        f"cd {tmp_dir} && sh ./install.sh "
        )
    if ret_code != 0:
        return ret_code

    print("Installing swupdate dependencies ...")

    print("  - Installing zstd APT package ...")
    ret_code = run_cmd(
        f"cd {tmp_dir} && apt-get -y install zstd "
        )
    if ret_code != 0:
        return ret_code

    print("  - Installing build-essential APT package "
          "(this will take a couple minutes) ...")
    ret_code = run_cmd(
        f"cd {tmp_dir} && apt-get -y install build-essential "
        )
    if ret_code != 0:
        return ret_code

    print("  - Installing libconfig-dev APT package ...")
    ret_code = run_cmd(
        f"cd {tmp_dir} && apt-get -y install libconfig-dev "
        )
    if ret_code != 0:
        return ret_code

    print("Setup swupdate .swu sw-versions and hwrevision ... ")
    ret_code = run_cmd(
        f"echo 'myboard 0.1' > /etc/hwrevision"
        )
    if ret_code != 0:
        return ret_code

    ret_code = run_cmd(
        f"echo 0.1 > /etc/sw-versions"
        )
    if ret_code != 0:
        return ret_code

    print("Running swupdate check on bugbash .swu  ...")

    ret_code = run_cmd(
        f"cd {tmp_dir} && sh ./check_swu.sh "
        )
    if ret_code != 0:
        return ret_code

    print("Doing swupdate dry-run of bugbash .swu  ...")

    ret_code = run_cmd(
        f"cd {tmp_dir} && sh ./apply_dryrun_swu.sh "
        )
    if ret_code != 0:
        return ret_code

    global content_dir
    print(f"copying swupdate-related files to {content_dir} ...")

    ret_code = run_cmd(
        f"mkdir -p {content_dir} "
        f"&& cd {tmp_dir} && cp ./*.pem ./bugbash_amd64.swu "
        f"./swupdatev2_handler_script.sh "
        f"./check_swu.sh ./apply_dryrun_swu.sh {content_dir} "
        )
    if ret_code != 0:
        return ret_code

    print("Installing zstd package for delta update target .swu ...")

    # install zstd APT package to allow swupdate to decompress
    # *.ext4.zst in deltaupdate recompressed target .swu
    ret_code = run_cmd(f"apt-get -y install zstd")
    if ret_code != 0:
        return ret_code

    return 0


def install_deltaupdate_files(script_dir):
    """Installs deltaupdate tools from the bundle"""

    print("Installing DeltaUpdate DeltaProcessor and DiffGen tools ...")

    # Install .net core 5 runtime for DiffGen Tools

    print("DiffGen - Installing .NET core 5 runtime ...")

    print("DiffGen .netcore5 - download packages-microsoft-prod.deb")
    ret_code = run_cmd("wget https://packages.microsoft.com/config/ubuntu/"
                       "18.04/packages-microsoft-prod.deb "
                       "-O packages-microsoft-prod.deb"
                       )
    if ret_code != 0:
        return ret_code

    print("DiffGen .netcore5 - dpkg install packages-microsoft-prod.deb")
    ret_code = run_cmd("dpkg -i packages-microsoft-prod.deb")
    if ret_code != 0:
        return ret_code

    print("DiffGen .netcore5 - apt update")
    ret_code = run_cmd("apt-get update")
    if ret_code != 0:
        return ret_code

    print("DiffGen .netcore5 - apt install apt-transport-https")
    ret_code = run_cmd("apt-get install -y apt-transport-https")
    if ret_code != 0:
        return ret_code

    print("DiffGen .netcore5 - apt update")
    ret_code = run_cmd("apt-get update")
    if ret_code != 0:
        return ret_code

    print("DiffGen .netcore5 - apt install aspnetcore-runtime-5.0")
    ret_code = run_cmd("apt-get install -y aspnetcore-runtime-5.0")
    if ret_code != 0:
        return ret_code

    # unzip
    filename = 'deltaupdate_files.zip'
    zip_file = os.path.join(script_dir, filename)

    global content_dir

    print(f"DiffGen - Ensure unzip APT pkg is installed ...")

    ret_code = run_cmd(f"apt-get -y install unzip")
    if ret_code != 0:
        return ret_code

    if not os.path.exists(content_dir):
        print(f"creating {content_dir} ...")
        ret_code = run_cmd(f"mkdir -p {content_dir}")
        if ret_code != 0:
            return ret_code

    print(f"DiffGen - copy {zip_file} to {content_dir} ...")

    ret_code = run_cmd(f"cp {zip_file} {content_dir}")
    if ret_code != 0:
        return ret_code

    print(f"DiffGen - unzip {content_dir}/{filename} ...")

    ret_code = run_cmd(f"cd {content_dir} && unzip ./{filename}")
    if ret_code != 0:
        return ret_code

    # install delta processor library
    #

    print("DeltaProcessor - Setup Ubuntu Toolchain PPA for newer glibc ...")

    ret_code = run_cmd("sudo apt-get -y install software-properties-common")
    if ret_code != 0:
        return ret_code

    ret_code = run_cmd(
        "sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test"
        )
    if ret_code != 0:
        return ret_code

    print("DeltaProcessor - Installing gcc-9 g++-9 Dependencies "
          "for loading libadudiffapi.so ..."
          )

    ret_code = run_cmd("apt-get -y install gcc-9 g++-9")
    if ret_code != 0:
        return ret_code

    lib_filename = 'libadudiffapi.so'
    delta_processor_lib_filepath = os.path.join(
        content_dir,
        'DeltaProcessor_x64',
        lib_filename
        )

    print("installing Delta processor library "
          f"{delta_processor_lib_filepath} ...")

    ret_code = run_cmd(
        f"cp {delta_processor_lib_filepath} /usr/local/lib/ "
        f"&& ldconfig /usr/local/lib/{lib_filename}"
        )
    if ret_code != 0:
        return ret_code

    print("unzip DiffGenTool ...")
    ret_code = run_cmd(
        "cd /tmp/deltaupdates/FIT_DiffGenTool "
        "&& unzip diffgen-tool.Release.x64-linux.zip"
        )
    if ret_code != 0:
        return ret_code

    print("Make DiffGen binaries executable ...")

    ret_code = run_cmd(
        "cd /tmp/deltaupdates/FIT_DiffGenTool/"
        "diffgen-tool.Release.x64-linux "
        "&& chmod 755 applydiff bsdiff bspatch compress_files.py DiffGenTool "
        "dumpdiff dumpextfs recompress_and_sign_tool.py "
        "recompress_tool.py sign_tool.py zstd_compress_file"
        )
    if ret_code != 0:
        return ret_code

    return 0


def bootstrap_system() -> int:
    """Bootstrap using expanded bundle in the dir of this script."""
    print(f"{os.linesep}Attempting to bootstrap the system ...")

    script_dir = os.path.dirname(sys.argv[0])

    ret_val = run_cmd("apt-get update")
    if ret_val != 0:
        return ret_val

    ret_val = install_do(script_dir)
    if ret_val != 0:
        return ret_val

    ret_val = install_du_agent(script_dir)
    if ret_val != 0:
        return ret_val

    ret_val = install_deltaupdate_swupdate_files(script_dir)
    if ret_val != 0:
        return ret_val

    ret_val = install_deltaupdate_files(script_dir)
    if ret_val != 0:
        return ret_val

    # update user and group onwership of /etc/adu/du-config.json if it exists
    # now that adu user and group have been setup so that healthcheck passes.
    if os.path.exists('/etc/adu/du-config.json'):
        ret_val = run_cmd('chown adu:adu /etc/adu/du-config.json')
        if ret_val != 0:
            return ret_val

    print("Very last step - Running DU Agent healthcheck "
          "'/usr/bin/AducIotAgent -h' ...")

    ret_code = run_cmd('/usr/bin/AducIotAgent -h')
    if ret_code != 0:
        return ret_code

    return 0


def download_file(url: str, target_dir: str) -> int:
    parsed_url = urlparse(url)
    last_slash = parsed_url.path.rindex('/')
    if last_slash == -1:
        print(f"download_file - No '/' found in parsed URL of '{url}'.")
        return 1
    filename = parsed_url.path[last_slash + 1:]
    target_path = os.path.join(target_dir, filename)

    if Verbose:
        print(f"Downloading URL '{url}' to '{target_path}' ...")

    req = requests.get(url)

    with open(target_path, 'wb') as file:
        file.write(req.content)

    return 0


def copy_deviceupdate_agent_files(cfg_duagent, work_dir) -> int:
    """Copy files for deviceupdate agent into work dir"""
    # Copy agent debian package
    src_file = cfg_duagent['package_path']
    if not os.path.exists(src_file):
        print("config deviceupdate_agent.package_path "
              f"'{src_file}' does not exist.")
        return 1

    try:
        shutil.copy(src_file, work_dir)

        # Copy DO release tarball for Ubuntu 18.04
        deps1804 = cfg_duagent['dependencies']['ubuntu18.04_x64']
        download_file(
            url=deps1804['delivery_optimization_tarball_url'],
            target_dir=work_dir
            )

        # Copy DO release tarball for Ubuntu 20.04
        deps2004 = cfg_duagent['dependencies']['ubuntu20.04_x64']
        download_file(
            url=deps2004['delivery_optimization_tarball_url'],
            target_dir=work_dir
            )
    except Exception as ex:
        print(ex)
        return 2

    return 0


def copy_feature_deltaupdate_files(cfg_deltaupdate, work_dir):
    """Copy files for delta update feature into work_dir"""
    deltaupdate_files_zip = cfg_deltaupdate['deltaupdate_files']
    if not os.path.exists(deltaupdate_files_zip):
        print("config deltaupdate_files "
              f"'{deltaupdate_files_zip}' does not exist.")
        return 1

    try:
        shutil.copy(deltaupdate_files_zip, work_dir)
    except Exception as ex:
        print(ex)
        return 2

    return 0


def copy_feature_deltaupdate_swupdate_files(cfg_deltaupdate, work_dir):
    """Copy custom swupdate-related files into work_dir"""
    swupdate_tarball = cfg_deltaupdate['deltaupdate_swupdate_files']
    if not os.path.exists(swupdate_tarball):
        print("config swupdate_tarball "
              f"'{swupdate_tarball}' does not exist.")
        return 1

    try:
        shutil.copy(swupdate_tarball, work_dir)
    except Exception as ex:
        print(ex)
        return 2
    return 0


def create_bootstrap_bundle(
        name: str,
        config: str,
        out_dir: str, work_dir: str
        ) -> int:
    """Create bootstrap bundle of given name using the config."""

    global Verbose

    print(
        f"{os.linesep}Creating bootstrap bundle '{name}.tar.gz'"
        f"{os.linesep}  using config '{config}' ..."
        )

    # First, validate the example config json against the jsonschema.
    is_valid, msg = validate_config(get_config_example())
    if not is_valid:
        print(msg)
        return 1

    if not os.path.exists(config):
        print(f"config '{config}' does not exist.")
        return 1

    mkdirs_if_not_exist(out_dir)
    mkdirs_if_not_exist(work_dir)

    with open(config, 'r') as config_file:
        config_json = json.load(config_file)

    if Verbose:
        print(json.dumps(config_json, indent=4))

    # Copy files into work_dir
    copy_deviceupdate_agent_files(config_json['deviceupdate_agent'], work_dir)
    copy_feature_deltaupdate_files(
        config_json['features']['delta_update'],
        work_dir
    )
    copy_feature_deltaupdate_swupdate_files(
        config_json['features']['delta_update'],
        work_dir
    )

    # copy bootstrap scripts into bundle as well
    script_dir = os.path.dirname(sys.argv[0])
    try:
        shutil.copy(sys.argv[0], work_dir)
        shutil.copy(os.path.join(script_dir, "bootstrap.sh"), work_dir)
    except Exception as ex:
        print(ex)
        return 2

    # Create bootstrap bundle tarball
    output_bundle_filepath = os.path.join(out_dir, f"{name}.tar.gz")
    if Verbose:
        print("Bundling following files into {output_bundle_filepath} ...")

    return run_cmd(f"cd {work_dir} && tar -czvf {output_bundle_filepath} ./*")


def main() -> int:
    """Either create bootstrap bundle or bootstrap the system."""

    # parse cmdline arguments
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="Creates bootstrap bundle or bootstraps the system.",
        epilog='''Example usage:

---Create bootstrap bundle---
./bootstrap.sh \\
    --bundle-name bundle_name \\
    --config /tmp/adu/boot/bootstrap_config.json \\
    --out-dir /tmp/adu/boot/out/ \\
    --work-dir /tmp/adu/boot/work/

---Run Bootstrap---
./bootstrap.sh --install
''')

    parser.add_argument('-i', '--install', action='store_true',
                        help="Installs the bootstrap bundle to the system.")
    parser.add_argument('-n', '--bundle-name', nargs='?',
                        help="The bootstrap bundle name.")
    parser.add_argument('-c', '--config', nargs='?',
                        help="The path to the config file.")
    parser.add_argument('-o', '--out-dir', nargs='?',
                        help="The output dir for the bundle tarball.")
    parser.add_argument('-w', '--work-dir', nargs='?',
                        help="The work dir for bundle creation.")
    parser.add_argument('-v', '--verbose', action='store_true',
                        help="Enables verbose tracing.")

    args = parser.parse_args()

    if args.verbose:
        global Verbose
        Verbose = True

    ret_val = 1
    if args.bundle_name is None \
            and args.config is None \
            and args.out_dir is None \
            and args.work_dir is None \
            and args.install is True:
        ret_val = bootstrap_system()
    elif args.bundle_name is not None \
            and args.config is not None \
            and args.out_dir is not None \
            and args.work_dir is not None:
        ret_val = create_bootstrap_bundle(
            args.bundle_name,
            args.config,
            args.out_dir,
            args.work_dir
            )
    else:
        parser.print_help()

    print(f"Completed with return code {ret_val}.")

    return ret_val


if __name__ == '__main__':
    sys.exit(main())
