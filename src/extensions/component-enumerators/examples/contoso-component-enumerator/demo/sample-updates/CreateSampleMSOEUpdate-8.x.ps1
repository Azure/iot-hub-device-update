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
        PS > CreateSampleMSOEUpdate-8.x.ps1 -UpdateProvider "Contoso" `
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
# Update : 8.0
#
# Create a parent update that updates 'hostfw' component, reboot a device, then update 'motors' and 'cameras' group respectively.
#
##############################################################

$UpdateVersion = '8.0'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."
    # -------------------------------------------------
    # Create a child update for hostfw
    # -------------------------------------------------
    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-hostfw"
    $RefUpdateVersion = "1.1"

    $hostfwFirmwareVersion = "1.1"

    Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $hostfwUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to a component named 'hostfw'.
    $hostfwSelector = @{ name = 'hostfw' }
    $hostfwCompat = New-AduUpdateCompatibility  -Properties $hostfwSelector

    $hostfwScriptFile = "$PSScriptRoot\scripts\contoso-firmware-installscript.sh"
    $hostfwFirmwareFile = "$PSScriptRoot\data-files\host-firmware-$hostfwFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #

    $hostfwInstallSteps = @()

    # Step #0 - install host firmware and reboot
    $hostfwInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $hostfwScriptFile, $hostfwFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-firmware-installscript.sh';  `
                                'installedCriteria'="$hostfwFirmwareVersion";   `
                                'arguments'="--restart-to-apply --firmware-file host-firmware-$hostfwFirmwareVersion.json  --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Update Host Firmware then restart a device'

    # ------------------------------
    # Create child update manifest
    # ------------------------------

    $childUpdateId = $hostfwUpdateId
    $childUpdateIdStr = "$($childUpdateId.Provider).$($childUpdateId.Name).$($childUpdateId.Version)"
    $childPayloadFiles = $hostfwScriptFile, $hostfwFirmwareFile
    $childCompat = $hostfwCompat
    $childSteps = $hostfwInstallSteps

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

    # -------------------------------------------------
    # Create a child update for every component in a 'motors' group.
    # -------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-motors"
    $RefUpdateVersion = "8.0"

    $motorsFirmwareVersion = "3.1"

    Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $motorsUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'motors' group.
    $motorsSelector = @{ group = 'motors' }
    $motorsCompat = New-AduUpdateCompatibility  -Properties $motorsSelector

    $motorScriptFile = "$PSScriptRoot\scripts\contoso-motor-installscript.sh"
    $motorFirmwareFile = "$PSScriptRoot\data-files\motor-firmware-$motorsFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #

    $motorsInstallSteps = @()

    # Step #0 - simulating a success pre-install task.
    $motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $motorScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-motor-installscript.sh';  `
                                'installedCriteria'="$motorsFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Motors Update pre-install step'

    # Step #1 - install a mock firmware version 1.2 onto motor component.
    $motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $motorScriptFile, $motorFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-motor-installscript.sh';  `
                                'installedCriteria'="$motorsFirmwareVersion"; `
                                "arguments"="--firmware-file motor-firmware-$motorsFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Motors Update - firmware installation'


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
    $outputPath = "$OutputManifestPath\$parentUpdateIdStr"
    Write-Host "    Saving child manifest files and payload to $outputPath..."
    New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

    # Generate manifest files.
    $childManifest | Out-File "$outputPath\$childUpdateIdStr.importmanifest.json" -Encoding utf8

    # Copy all payloads (if used)
    Copy-Item -Path $childPayloadFiles -Destination $outputPath -Force

    Write-Host " "


    # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    # -------------------------------------------------
    # Create a child update for every component in a 'cameras' group
    # -------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-cameras"
    $RefUpdateVersion = "8.0"

    $camerasFirmwareVersion = "3.0"

    Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'cameras' group.
    $camerasSelector = @{ group = 'cameras' }
    $camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

    $cameraScriptFile = "$PSScriptRoot\scripts\contoso-camera-installscript.sh"
    $cameraFirmwareFile = "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #

    $camerasInstallSteps = @()

    # Step #0 - simulating a success pre-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update pre-install step'

    # Step #1 - install a mock firmware version 2.1 onto camera component.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile, $cameraFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--firmware-file camera-firmware-$camerasFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - firmware installation'

    # ------------------------------
    # Create child update manifest
    # ------------------------------

    $childUpdateId = $camerasUpdateId
    $childUpdateIdStr = "$($childUpdateId.Provider).$($childUpdateId.Name).$($childUpdateId.Version)"
    $childPayloadFiles = $cameraScriptFile, $cameraFirmwareFile
    $childCompat = $camerasCompat
    $childSteps = $camerasInstallSteps

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

# ----------------------------
# Create the parent update
# ----------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr ..."

$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    $parentSteps += New-AduInstallationStep -UpdateId $hostfwUpdateId -Description "Host Firmware Version $hostfwFirmwareVersion"
    $parentSteps += New-AduInstallationStep -UpdateId $motorsUpdateId -Description "Motors Firmware Version $motorsFirmwareVersion"
    $parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Version $camerasFirmwareVersion"

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
