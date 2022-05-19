#
# Device Update for IoT Hub
# Sample PowerShell script for creating import manifests and payloads files for testing purposes.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create a sample updates for testing purposes.

    .EXAMPLE
        PS > CreateSampleMSOEUpdate-4.x.ps1 -UpdateProvider "Contoso" `
                                             -UpdateName "Virtual-Vacuum" `
                                             -DeviceManufacturer "contoso" `
                                             -DeviceModel "virtual-vacuum-v1" `
                                             -OutputManifestPath .
#>
[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputManifestPath,

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = "Contoso",

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = "Virtual-Vacuum",

    # Device Manufacturer
    [ValidateNotNullOrEmpty()]
    [string] $DeviceManufacturer = "contoso",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $DeviceModel = "virtual-vacuum-v1"
)

##############################################################
# Update : 4.0
#
# Create the parent update containing 1 reference update
#     - Child update contains 3 step. First step failed.
#     - Child update is for an optional steamer components.
##############################################################

$UpdateVersion = '4.0'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."

    # -----------------------------------------------------------
    # Create a child update for an optional 'steamer' component
    # that currently not connected to the host device.
    # -----------------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-steamers"
    $RefUpdateVersion = "1.0"

    $steamersFirmwareVersion = "1.0"

    Write-Host "    Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $steamersUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'steamers' group.
    $steamersSelector = @{ group = 'steamers' }
    $steamersCompat = New-AduUpdateCompatibility  -Properties $steamersSelector

    $steamerScriptFile = "$PSScriptRoot\scripts\contoso-steamer-installscript.sh"
    $steamerFirmwareFile = "$PSScriptRoot\data-files\steamer-firmware-$steamersFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #

    # This update contains 3 steps.
    $steamersInstallSteps = @()

    # Step #1 - simulating a success pre-install task.
    $steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $steamerScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-steamer-installscript.sh';  `
                                'installedCriteria'="$steamersFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Steamers Update pre-install step'

    # Step #2 - install a mock firmware version 1.0 onto steamer component.
    $steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $steamerScriptFile, $steamerFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-steamer-installscript.sh';  `
                                'installedCriteria'="$steamersFirmwareVersion"; `
                                "arguments"="--firmware-file steamer-firmware-$steamersFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Steamers Update - firmware installation'

    # Step #3 - simulating a success post-install task.
    $steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $steamerScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-steamer-installscript.sh';  `
                                'installedCriteria'="$steamersFirmwareVersion"; `
                                "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                            }   `
                            `
                            -Description 'Motors Update post-install step'

    # ------------------------------
    # Create child update manifest
    # ------------------------------

    $childUpdateId = $steamersUpdateId
    $childUpdateIdStr = "$($childUpdateId.Provider).$($childUpdateId.Name).$($childUpdateId.Version)"
    $childPayloadFiles = $steamerScriptFile, $steamerFirmwareFile
    $childCompat = $steamersCompat
    $childSteps = $steamersInstallSteps

    Write-Host "    Preparing child update manifest $childUpdateIdStr ..."

    $childManifest = New-AduImportManifest -UpdateId $childUpdateId -IsDeployable $false `
                                        -Compatibility $childCompat `
                                        -InstallationSteps $childSteps `
                                        -ErrorAction Stop

    # Create folder for manifest files and payload.
    $outputPath = "$OutputManifestPath\$parentUpdateIdStr"
    Write-Host "    Saving child manifest files and payload to $outputPath..."
    New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

    # Generate manifest files.
    $childManifest | Out-File "$outputPath\$childUpdateIdStr.importmanifest.json" -Encoding utf8

    # Copy all payloads (if used)
    Copy-Item -Path $childPayloadFiles -Destination $outputPath -Force

    Write-Host " "

# ------------------------------------------------------
# Create the parent update containing one reference step
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - A reference step for steamers update.
    $parentSteps += New-AduInstallationStep `
                        -UpdateId $steamersUpdateId `
                        -Description "Steamer Firmware Update"

# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."

$parentManifest = New-AduImportManifest -UpdateId $parentUpdateId `
                                    -IsDeployable $true `
                                    -Compatibility $parentCompat `
                                    -InstallationSteps $parentSteps `
                                    -ErrorAction Stop

# Create folder for manifest files and payload.
$outputPath = "$OutputManifestPath\$parentUpdateIdStr"
Write-Host "    Saving parent manifest file and payload(s) to $outputPath..."
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest file.
$parentManifest | Out-File "$outputPath\$parentUpdateIdStr.importmanifest.json" -Encoding utf8

# Copy all payloads (if used)
if ($payloadFiles) { Copy-Item -Path $payloadFiles -Destination $outputPath -Force }

Write-Host " "
