
#
# Device Update for IoT Hub
# Sample PowerShell script for creating import manifests and payloads files for tutorial #1.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create a sample updates for testing purposes.

    .EXAMPLE
        PS > tutorial1.ps1
#>

# -------------------------------------------------
# Create the first top-level step
# -------------------------------------------------

$topLevelStep1 = New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files "./update-files/apt-manifest-tree-1.0.json" `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-tree-1.0'} `
                        -Description 'Install tree Debian package on host device.'


# -------------------------------------------------
# Create a child update for all 'motor' components
# -------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "contoso-virtual-motors"
$RefUpdateVersion = "1.1"

$motorsFirmwareVersion = "1.1"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$motorsUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'motors' group.
$motorsSelector = @{ group = 'motors' }
$motorsCompat = New-AduUpdateCompatibility  -Properties $motorsSelector

$motorScriptFile = ".\update-files\contoso-motor-installscript.sh"
$motorFirmwareFile = ".\update-files\motor-firmware-$motorsFirmwareVersion.json"

#------------
# ADD STEP(S)
#

# This update contains 1 steps.
$motorsInstallSteps = @()

# Step #1 - install a firmware version 1.1 onto motor component.
$motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files $motorScriptFile, $motorFirmwareFile `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-motor-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file motor-firmware-$motorsFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Motors Update - firmware 1.1 installation'

# ------------------------------
# Create child update manifest
# ------------------------------

$childUpdateId = $motorsUpdateId
$childUpdateIdStr = "$($childUpdateId.Provider).$($childUpdateId.Name).$($childUpdateId.Version)"
$childPayloadFiles = $motorScriptFile, $motorFirmwareFile
$childCompat = $motorsCompat
$childSteps = $motorsInstallSteps

Write-Host "    Preparing child update manifest $childUpdateIdStr ..."

$childManifest = New-AduImportManifest -UpdateId $childUpdateId -IsDeployable $false `
                                    -Compatibility $childCompat `
                                    -InstallationSteps $childSteps `
                                    -ErrorAction Stop

# Create folder for manifest files and payload.
$outputPath = ".\update-files"
Write-Host "    Saving child manifest files and payload to $outputPath..."
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest files.
$childManifest | Out-File "$outputPath\$childUpdateIdStr.importmanifest.json" -Encoding utf8


#
# Create a top-level reference step.
#
$topLevelStep2 = New-AduInstallationStep `
                        -UpdateId $childUpdateId `
                        -Description "Motor Firmware 1.1 Update"

##############################################################
# Update : 20.0
##############################################################

# Update Identity
$UpdateProvider = "Contoso"
$UpdateName = "Virtual-Vacuum"
$UpdateVersion = '20.0'

# Host Device Info
$Manufacturer = "contoso"
$Model = 'virtual-vacuum-v1'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"


# ------------------------------------------------------
# Create the parent update containing 1 inline step and 1 reference step.
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - Install 'tree' package
    $parentSteps += $topLevelStep1

    # step #2 - Install 'motors firmware'
    $parentSteps += $topLevelStep2

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
$outputPath = ".\update-files"
Write-Host "    Saving parent manifest file and payload(s) to $outputPath..."
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest file.
$parentManifest | Out-File "$outputPath\$parentUpdateIdStr.importmanifest.json" -Encoding utf8
