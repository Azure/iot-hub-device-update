# Example : OTA updating Ubuntu Core snaps

This example showcases the complete workflow for updating a snap package on an Ubuntu Core device using Device Update for IoT Hub.

For your convenience, we have provided the following files as references:

| File | Description |
|---|---|
| [create-import-manifest.ps1](./create-import-manifest.ps1) | A PowerShell script that creates the import manifest file for the update package. |
| [example-snap-update.sh](./example-snap-update.sh) | A Bash script that is required by the Step Handler extension of type `microsoft/script:1`'|

## Prerequisites

Before proceeding, make sure you have a basic understanding of how the [Script Handler](../../../../src/extensions/step_handlers/script_handler/README.md) works and how to [import an update package](https://learn.microsoft.com/azure/iot-hub-device-update/import-concepts) to the Device Update for IoT Hub.

## Write a Bash script for the Script Handler

> IMPORTANT NOTE | The script given is a general guideline, but it may be necessary to make some enhancements, such as implementing error-handling and security measures, for the script to be suitable for use in production devices. 


As mentioned earlier, the complete fully functional Bash script ([example-snap-update.sh](./example-snap-update.sh)) is provided for your convenience. You can chose to skip to the next step to generate the import manifest.

In a nutshell, this script implements 2 important functions for this update scenario.

| Function | Responsibility |
|---|---|
| CheckIsInstalledState | This function determines whether the snap package, as specified 'installedCriteria' (parameter $1) is currrnetly installed on the device. |
| InstallUpdate | This function installs a specific snap package by making a request to the snapd RESTful API and waiting for the installation to complete, if necessary, while logging any errors that occur. |

Although the following functions are required, they do not perform any actions for this update scenario and will return success codes.

| Function | Responsibility |
|---|---|
| DownloadUpdateArtifacts | If an additional files is neeed for your update scenario, you can download them with in this function.<br/>For this demo, no additional files is needed. So this function is no-op and will return download success code. |
| ApplyUpdate | This function doesn't do anything and will always return a default success result.|
| CancelUpdate | For this demonstration, cancellation of the update is not possible. If the snap has already been updated or installed due to the update, it cannot be undone.|


### Determine whether a snap package is already installed

Determining whether a Snap is installed on a device can be accomplished using Snapd, the daemon responsible for managing the lifecycle of Snaps on a Linux system. Snapd provides a RESTful API that can be used to query information about installed Snaps, including their version and channel.

The `CheckIsInstalledState` function determines whether a specified snap package is currently installed on the device based on its [installedCriteria](#installed-criteria). 

The `is_installed` helper function uses the Snapd RESTful API to check if the package is installed by sending a GET request to the URL "http://localhost/v2/snaps/$snap_name?select=version,channel".  

If the package is installed, the Snapd RESTful API returns a status code of 200, and the function compares the installed version and channel with the expected version and channel specified in installedCriteria. If the installed version and channel match the expected values, the is_installed function returns a result code of 900 (installed) and an empty result details string.  

If the package is not installed or if the installed version and channel do not match the expected values, the function returns a result code of 901 (not installed) and a result details string describing the error. 

If the Snapd RESTful API returns a status code of 404 or an unexpected status code, the function returns a result code of 901 and a result details string describing the error. 

Finally, CheckIsInstalledState prepares and saves the ADUC result and sets the return value to 0.

## Installed Criteria

The installedCriteria string is used to specify the name, version, and channel of the package to be installed on the device. The string is a comma-separated list of values that correspond to the following parameters:

|||
|---|---|
| snap_name | The name of the snap package to be installed. |
| snap_version | (Optional) The version of the snap package to be installed. If this parameter is not provided, the latest version of the snap package will be installed. |
| snap_channel |(Optional) The channel of the snap package to be installed. If this parameter is not provided, the package will be installed from the default channel. | 

For example, the string audacity,,stable specifies that the audacity package should be installed from the stable channel without specifying a specific version. The string my-snap,1.2.3,beta specifies that version 1.2.3 of the my-snap package should be installed from the beta channel. 

### Install a snap package

The `InstallUpdate` function is used to install a specific snap package by making a request to the Snapd RESTful API. The function takes a comma-delimited string called `installed_criteria` that contains the snap name, version, and channel. 

The function then builds a JSON object containing the necessary data to make a POST request to the snapd daemon. This object includes the action of installing the snap and any optional revision and channel values specified in the `installed_criteria` string. The JSON object is then sent as data in the POST request to the snapd RESTful API endpoint for snap installation.

If the snap package is already installed, the function logs an information message and returns the result code for "already installed" (603). 

If the package is not installed, the function sends a POST request to the snapd daemon to install the package. The function checks the status code of the response from the snapd daemon to determine the status of the installation.

If the installation is in progress, the function logs an information message and waits for the installation to complete. The function checks the status of the installation periodically by sending another POST request to the snapd daemon and examining the response body. Once the installation is complete, the function logs an information message and returns the result code for successful installation (600).

In summary, the function uses the snapd RESTful API to send a POST request to the snapd daemon to install a specific snap package, and checks the status of the installation by examining the response body from the snapd daemon.

## Understanding the communication between the Script Handler and the update script

During the update process, if a step with the handler property set to `microsoft/script:1` is included in the update manifest, the DU Agent workflow will use the `Step Handler` to process that update step. The Step Handler is configured using the `handlerProperties`, a collection of name/value pairs, specified by the update creator.

The Step Handler loads the Bash script specified in the scriptFileName property, such as example-snap-update.sh, and runs it in a forked child process. It passes important information such as the work folder, output file, result file, log file, installed criteria, and the maximum installation wait time to the script.

It is important to note that regardless of whether the update is successful or not, the script must write the ADUC_Result data in JSON format to the specified result file.

See script usage message below for more information

```shell
Usage: example-snap-update.sh [options...]


Device Update reserved argument
===============================

--action <ACTION_NAME>            Perform the specified action.
                                  Where ACTION_NAME is is-installed, download, install, apply, cancel, backup and restore.

--action-is-installed             Perform 'is-installed' check.
                                  Check whether the selected component [or primary device] current states
                                  satisfies specified 'installedCriteria' data.
--installed-criteria              Specify the Installed-Criteria string.

--max-install-wait-time           The timeout period for snap installation is the maximum wait time before giving up, in seconds.
                                  Default wait time is 4 hours.

--action-download                 Perform 'download' aciton.
--action-install                  Perform 'install' action.
--action-apply                    Perform 'apply' action.
--action-cancel                   Perform 'cancel' action.

File and Folder information
===========================

--work-folder             A work-folder (or sandbox folder).
--output-file             An output file.
--log-file                A log file.
--result-file             A file contain ADUC_Result data (in JSON format).

--log-level <0-4>         A minimum log level. 0=debug, 1=info, 2=warning, 3=error, 4=none.
-h, --help                Show this help message.
```

## Testing The Script Locally

It's a good practice to test your script on a test device locally before importing and deploying it via Device Update deployment to ensure that the script is functioning as expected.

Since this script will be invoked by DU Agent running on the device (using Script Handler), the following options will be passed to the script:

| Option | Argument(s) | Description |
|---|---|---|
| --work-folder | PATH_TO_WORK_FOLDER | The work folder is created at the beginning of the update workflow and is deleted once the update workflow is completed.|
| --log-file | PATH_TO_LOG_FILE | If you want the log to be included in the DU Agent logs, the script should write to the designated log file. |
| --output-file | PATH_TO_OUTPUT_FILE | To enhance diagnostic capabilities, you can print additional information to the output file.  |
| --result-file | PATH_TO_RESULT_FILE | The result file is an essential component as it contains the resultCode, extendedResultCode, and resultDetails required by the Script Handler to determine the success status of the action. Regardless of the action result, the result file is expected to exist after the completion of the script.  |

For testing purposes, you can use the following values:

```shell
# Test environment
test_id=1

test_work_folder=$SNAP_DATA/test-work-folder
mkdir -p $test_work_folder

log_file="$test_work_folder/test_$test_id.log"
output_file="$test_work_folder/test_$test_id.output.txt"
result_file="$test_work_folder/test_$test_id.result.json"

# Usage
./example-snap-update.sh --work-folder $test_work_folder --log-file "$log_file" --output-file "$output_file" --result-file "$result_file" ...
```

### Test `is-installed` action

To test `is-installed` action, follwing options are required:

| Option | Argument(s) | Description |
|---|---|---|
| --action-is-installed | None | Indicates that we're invoking the `is-installed` action. The following `--installed-criteria` option is required. |
| --installed-criteria | INSTALLED_CRITERIA | INSTALLED_CRITERIA is a comma-delimited string that contains three tokens: `snap_name`, `snap_version`, and `snap_channel`. These tokens are used to specify the name, version, and channel of a software package to be installed on a Linux system.<br/> The snap_name token is required and specifies the name of the package to be installed. The snap_version token is optional and specifies the version of the package to be installed. The snap_channel token is also optional and specifies the release channel from which the package should be installed.<br/> An example value for installed_criteria could be my-snap,1.0.0,beta, where my-snap is the name of the package to be installed, 1.0.0 is the version of the package, and beta is the release channel from which the package should be installed.<br/> An example value for installed_criteria could be `my-snap,1.0.0,beta`, where my-snap is the name of the package to be installed, 1.0.0 is the version of the package, and beta is the release channel from which the package should be installed.

#### Example Test Script

```shell
test_id=is_installed_1

test_work_folder=$SNAP_DATA/test-work-folder
mkdir -p $test_work_folder

log_file="$test_work_folder/test_$test_id.log"
output_file="$test_work_folder/test_$test_id.output.txt"
result_file="$test_work_folder/test_$test_id.result.json"

rm $log_file
rm $output_file
rm $result_file

# Test the script (installed snap)
./example-snap-update.sh --work-folder $test_work_folder --log-file "$log_file" --output-file "$output_file" --result-file "$result_file" --action-is-installed --installed-criteria "snapdiff,," && echo -e "\n### Output File ###\n\n$(cat $output_file)\n\n#### Log File ###\n\n$(cat $log_file)\n\n#### Aduc Result File ###\n$(cat $result_file)\n\n###########\n\n"
```

Example output:

```shel
### Output File ###

[2023/03/18:000955] Result: {"resultCode":901, "extendedResultCode":0,"resultDetails":"Package is not installed (status code 404)"}

#### Log File ###

[2023/03/18:000955] [I] IsInstalled Task ("snapdiff,,")

#### Aduc Result File ###
{"resultCode":901, "extendedResultCode":0,"resultDetails":"Package is not installed (status code 404)"}

###########
```


## Create An Update


### Install PowerShell

```shell
$ sudo snap install powershell
```

### Install Azure CLI

See [Install Azure CLI on Linux](https://learn.microsoft.com/en-us/cli/azure/install-azure-cli-linux?pivots=apt#option-2-step-by-step-installation-instructions) for step-by-step instructions.

### Generating the Import Manifest

```powershell
./create-import-manifest.ps1
```

The above command will generate an import manifest file named `Contoso-Jukebox-1.0.importmanifest.json`.

To import this update to the Device Update for IoT Hub, you'll need to upload this manifest file and `example-snap-update.sh` script file.

Let's take a closer look at the `create-import-manifest.ps1`

```powershell

$updVersion = '1.0'
$updMeta = @{ Provider="Contoso"; Name="Jukebox"; Version="$updVersion"}

$updCompat = @{ manufacturer = 'contoso'; model = 'jukebox-v1' }

$manifestFile = '{0}-{1}-{2}.importmanifest.json' -f $updMeta.Provider, $updMeta.Name, $updMeta.Version

$scriptFile= 'example-snap-update.sh'
$installedCriteria = 'audacity,,'

$handlerProperties = "{ 'scriptFileName': '$scriptFile', 'installedCriteria': '$installedCriteria' }"

az iot du update init v5 `
    --update-provider $updMeta.Provider `
    --update-name $updMeta.Name `
    --update-version $updMeta.Version `
    --description ('Install the latest audacity snap package from any available channel.') `
    --compat manufacturer=$($updCompat.manufacturer) model=$($updCompat.model)  `
    --step handler=microsoft/script:1  `
           properties=$handlerProperties `
           description="Install latest version of audacity snap." `
    --file path=./$scriptFile `
| Out-File -Encoding ascii $manifestFile

```

The script creates an import manifest file for use with the Device Update for IoT Hub service. It starts by defining some metadata about the update (provider, name, version), and compatibility information (manufacturer and model).

Next, it specifies the script file to be used in the update (example-snap-update.sh) and the installed criteria for the snap package (audacity,,).

The script then uses the Azure CLI (az iot du) to initialize the update and add a single update step to it. The step uses the microsoft/script:1 handler and passes the script file and installed criteria as properties. The description for the step indicates that it will install the latest version of the audacity snap package from any available channel.

Finally, the PowerShell script pipes the output of the az iot du command to a new file with a .importmanifest.json extension, which will be used to import the update into the Device Update for IoT Hub service.

Example Output Import Manifest:

```json
{
  "compatibility": [
    {
      "manufacturer": "contoso",
      "model": "jukebox-v1"
    }
  ],
  "createdDateTime": "2023-03-17T17:34:43Z",
  "description": "Install the latest audacity snap package from any available channel.",
  "files": [
    {
      "filename": "example-snap-update.sh",
      "hashes": {
        "sha256": "WDbdlOYJSlf0nYnc2vs85fhi+JFm8LX2aJzTbsswha4="
      },
      "sizeInBytes": 26942
    }
  ],
  "instructions": {
    "steps": [
      {
        "description": "Update snapdiff",
        "files": [
          "example-snap-update.sh"
        ],
        "handler": "microsoft/script:1",
        "handlerProperties": {
          "installedCriteria": "audacity,,",
          "scriptFileName": "example-snap-update.sh"
        },
        "type": "inline"
      }
    ]
  },
  "manifestVersion": "5.0",
  "updateId": {
    "name": "Jukebox",
    "provider": "Contoso",
    "version": "1.0"
  }
}
```
