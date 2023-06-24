
# Device Update for IoT Hub
#
# Sample PowerShell script creating an import manifest file for an Azure IoT Device Update package.
# Defines update metadata and compatibility information, as well as a script to be executed during the update.
# The script checks for and installs the latest version of the "audacity" snap package on the device.
#
# Copyright (c) Microsoft Corporation.
# License: MIT

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create an sample update that demonstrates how to install multiple files using a script handler.

    .EXAMPLE
        PS > create-import-manifest.ps1
#>

$updVersion = '1.0'
$updMeta = @{ Provider="Contoso"; Name="Databrick"; Version="$updVersion"}

$updCompat = @{ manufacturer = 'contoso'; model = 'databrick-v1' }

$manifestFile = '{0}-{1}-{2}.importmanifest.json' -f $updMeta.Provider, $updMeta.Name, $updMeta.Version

$scriptFile= 'example-script-handler-actions-impl.sh'
$payloadFile1= 'data-file-1'
$payloadFile2= 'data-file-2'
$payloadFile3= 'process-data-file.sh'

$installedCriteria = 'Hello World!!!'

## The following demonstrates how to specify a custom script handler property named "outputDataBrickFile" that will be passed to the script handler.
$handlerProperties = "{ 'scriptFileName': '$scriptFile', 'installedCriteria': '$installedCriteria', 'data-brick-file-name': 'data-3.brick' }"

az iot du update init v5 `
    --update-provider $updMeta.Provider `
    --update-name $updMeta.Name `
    --update-version $updMeta.Version `
    --description ('Install data-1 and data-2 files and generate data-3.brick') `
    --compat manufacturer=$($updCompat.manufacturer) model=$($updCompat.model)  `
    --step handler=microsoft/script:1  `
           properties=$handlerProperties `
           description="Install data-1 and data-2 files and generate data-3.brick" `
    --file path=./$scriptFile `
    --file path=./$payloadFile1 `
    --file path=./$payloadFile2 `
    --file path=./$payloadFile3 `
| Out-File -Encoding ascii $manifestFile
