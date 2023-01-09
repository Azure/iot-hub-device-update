# How To Test The Scripts And Generate Update(s)

## How To Test demo-a-b-rootfs-script.sh on Ubuntu Device or VM 

To manually test the script on the customized device (must have /dev/mmcblk0p2 and /dev/mmcblk0p3 as Root FS), set following environment variables, then try the [test commands]() below.

```sh
cd ~
wf=~/tmp
sudo mkdir -p $wf
logfile=$wf/demo-rootfs-installer.log
outfile=$wf/demo-ab-rootfs-update.out
resfile=$wf/demo-ab-rootfs-update.result.json
swverfile=$wf/demo-du-image-version
inst_criteria=ubuntu-20.04-adu-1.0.6
blfolder=$wf/firmware
sudo mkdir -p $blfolder
```

Below are environment variables that resembled the device built from our CustomPiOS project configuration

```sh
cd ~
wf=~/tmp
sudo mkdir -p $wf
logfile=$wf/demo-rootfs-installer.log
outfile=$wf/demo-ab-rootfs-update.out
resfile=$wf/demo-ab-rootfs-update.result.json
swverfile=/etc/du-image-version
inst_criteria=ubuntu-20.04-adu-1.0.6
blfolder=/boot/firmware
sudo mkdir -p $blfolder
```

## Test Commands

### Test --action-is-installed

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-is-installed

cat $resfile
```

### Test --action-download

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-download

cat $resfile
```

### Test --action-install (Performs actual file copy, to speed up the process)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-install 

cat $resfile
```

### Test --action-install (Skip file copy)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-install --debug-install-no-file-copy

cat $resfile
```

### Test --action-apply (no device restart)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-apply

cat $resfile
```

### Test --action-apply (Restart device after apply succeeded)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria"  --boot-loader-folder "$blfolder" --action-is-installed

cat $resfile
```

## How To Generate The Device Update Import Manifest and Payloads

To generate the import manifest and the update payloads that can be imported into the Device Update Service, follow this instruction:

- Open a PowerShell console
- Create a working folder
    ```powershell
    mkdir ~/myupdate
    ```
- Change to the working folder
    ```powershell
    pushd ~/myupdate
    ```
- Copy [AduUpdate.psm1](../../../tools/AduCmdlets/AduUpdate.psm1) to the working folder, then call `Import-Module`
    ```powershell
    cp <filelocation>/create-demo-a-b-rootfs-update-import-manifest.ps1 .
    cp -s <fileloction>/payloads ./
    ```
- Copy [create-demo-a-b-rootfs-update-import-manifest.ps1](./create-demo-a-b-rootfs-update-import-manifest.ps1) and [payloads](./payloads/) folder into the working folder
    ```powershell
    cp <filelocation>/create-demo-a-b-rootfs-update-import-manifest.ps1 .
    cp -s <fileloction>/payloads ./
    ```
- Generate the import manifest. You can change the update version number as needed. The period (.) at the end indicates that the output will be placed in the current folder.
    ```powershell
    ./create-demo-a-b-rootfs-update-import-manifest.ps1 -UpdateVersion "1.0.1" .
    ```
You should see the output like this:
```sh
#  
# Generating an import manifest
#
# Update version : 1.0.1
#
# Payloads :
#    \home\me\myupdate\payloads\demo-a-b-rootfs-script.sh
#
#    Output directory: fabrikam.rpi-ubuntu-2004-adu-update.1.0.1
#
#    Import manifest: .\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1.importmanifest.json
#    Saved payload(s) to: .\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1
#
```

## Where To Go From Here?

The provided script can be used as a starting point for your project. For example, this [demo-a-b-rootfs-script.sh](./payloads/demo-a-b-rootfs-script.sh) can be modified to work with `swupdate`, to support an installation of the `.swu` files.

> NOTE | `swupdate` tool must be installed on the device. This is not currently cover by this document. See [SW Update](http://swupdate.org/) official site for mor information.

Even though the example script seems too big to comprehend, most of the code are a boiler-plate that translates the data passed from the `Script Handler` to the script. The most important parts of the script are functions that implement the business-logic for each `update-related-action` (--action-is-install, --action-download, --action-install, --action-apply, and --action-cancel).

To customize the script to support your device configurations, it's easier to start at the following functions:

| Function | Description |
|---|---|
| CheckIsInstalledState | Determines whether the device has already installed the update. In most case, the `ScriptHandler` will pass the 'installed criteria' data to the script. This data is used for computing the 'installed' state. However, you can choose to determine the 'installed' state without any additional data from the `ScriptHandler`, if your scenario allowed.
|DownloadUpdate | Downloads any additional file(s) required by the current update. In some case, this is optional.
|InstallUpdate | Performs any file(s) copy, data transfer, or any modification to the device.
| ApplyUpdate | Performs any (optional) update finalization. For example, restart a system service, cron jobs, network stack, or even reboot the device.|
|CancelUpdate | Performs a cancellation. This function should make a best effort to restore the device to its last-known-good state (before any modification to the device are made)


### Example  - Modify `InstallUpdate` Function To Use SWUpdate tool

The following code snippet demonstrate how one could integrate `swupdate` as the installer.

```sh
#
# Performs the update installation.
#
# Before install any files, is_installed function is called to check whether the update is already installed on the target.
# If already installed, do nothing and report ADUC_Result_Install_Skipped_UpdateAlreadyInstalled (603) result code.
#
# Otherwise, try to install the update. If success, report ADUC_Result_Install_Success (600)
# Or, report ADUC_Result_Failure (0), if failed.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Install_Success = 600,                        /**< Succeeded. */
#    ADUC_Result_Install_Skipped_UpdateAlreadyInstalled = 603, /**< Succeeded (skipped). Indicates that 'install' action is no-op. */
#    ADUC_Result_Install_RequiredImmediateReboot = 605,        /**< Succeeded. An immediate device reboot is required, to complete the task. */
#    ADUC_Result_Install_RequiredReboot = 606,                 /**< Succeeded. A deferred device reboot is required, to complete the task. */
#    ADUC_Result_Install_RequiredImmediateAgentRestart = 607,  /**< Succeeded. An immediate agent restart is required, to complete the task. */
#    ADUC_Result_Install_RequiredAgentRestart = 608,           /**< Succeeded. A deferred agent restart is required, to complete the task. */
#
InstallUpdate() {
    log_info "InstallUpdate called"

    resultCode=0
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # Note: we could simulate 'component off-line' scenario here.
    #

    # Check whether the component is already installed the specified update...
    is_installed "$installed_criteria" "$software_version_file" resultCode extendedResultCode resultDetails

    is_installed_ret=$?

    if [[ $is_installed_ret -ne 0 ]]; then
        # is_installed function failed to execute.
        resultCode=0
        make_script_handler_erc "$is_installed_ret" extendedResultCode
        resultDetails="Internal error in 'is_installed' function."
    elif [[ $resultCode == 0 ]]; then
        # Failed to determine whether the update has been installed or not.
        # Return current ADUC_Result
        echo "Failed to determine wehther the update has been installed or note."
    elif [[ $resultCode -eq 901 ]]; then
        # Not installed.

        # install an update.
        echo "Installing update." >> "${log_file}"
        if [[ -f $image_file ]]; then

            # Note: swupdate can use a public key to validate the signature of an image.
            #
            # Here is how we generated the private key for signing the image
            # and how we generated that public key file used to validate the image signature.
            #
            # Generated RSA private key with password using command:
            # openssl genrsa -aes256 -passout file:priv.pass -out priv.pem
            #
            # Generated RSA public key from private key using command:
            # openssl rsa -in ${WORKDIR}/priv.pem -out ${WORKDIR}/public.pem -outform PEM -pubout

            if [[ ${public_key_file} -eq "" ]]; then
                # Call swupdate with the image file and no signature validations
                swupdate -v -i "${image_file}" -e ${selection} &>> "${log_file}"
            else
                # Call swupdate with the image file and the public key for signature validation
                swupdate -v -i "${image_file}" -k "${public_key_file}" -e ${selection} &>> "${log_file}"
            fi
            ret_val=$?

            if [[ $ret_val -eq 0 ]]; then
                resultCode=600
                extendedResultCode=0
                resultDetails=""
            else
                resultCode=0
                make_script_handler_erc "$ret_val" extendedResultCode
                resultDetails="SWUpdate command failed."
            fi
        else
            echo "Image file ${image_file} was not found." >> "${log_file}"
            resultCode=0
            # ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_IMAGE_FILE_NOT_FOUND (0x202)
            extendedResultCode=0x30100202
            resultDetails="Image file ${image_file} was not found."
        fi
        ret_val=0
    else
        # Installed.
        log_info "It appears that this update has already been installed."
        resultCode=603
        extendedResultCode=0
        resultDetails="Already installed."
        ret_val=0
    fi

    # Prepare ADUC_Result json.
    make_aduc_result_json "$resultCode" "$extendedResultCode" "$resultDetails" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

```

Notice how above code snipped leverage some of existing the parameters, such as `image_file`, `public_key_file`, `log_file`, etc.

Any additional data can be specified in the import manifest as part of the `HandlerProperties.arguments`. See example on how to specify additional arguments in [create-demo-a-b-rootfs-update-import-manifest.ps1](./create-demo-a-b-rootfs-update-import-manifest.ps1), like so:

```powershell
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/script:1'     `
                        -Files $payloadFiles  `
                        -HandlerProperties @{ `
                            'scriptFileName'="$DefaultScriptFileName"; `
                            'installedCriteria'="$InstalledCriteria"; `
                            'arguments'="--software-version-file /etc/du-image-version --log-file /var/log/adu/demo-rootfs-installer.log --boot-loader-folder /boot/firmware --restart-device-to-apply" `
                        } `
                        -Description 'Update the Root FS using A/B update strategy.'
```

For more information about `ScriptHandler`, please go [here](.././../../src/extensions/step_handlers/script_handler/README.md)
