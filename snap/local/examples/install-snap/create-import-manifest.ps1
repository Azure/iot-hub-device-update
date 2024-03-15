
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
        Create an sample update that install the latest 'audacity' Ubuntu Core snap package from any vailable channel.

    .EXAMPLE
        PS > create-import-manifest.ps1
#>

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
