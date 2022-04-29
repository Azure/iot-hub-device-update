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
        PS > CreateSampleMSOEUpdate-3.x.ps1 -UpdateProvider "Contoso" `
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
# Update : 3.0
#
# Create the parent update containing 1 reference update
#     - Child update contains 3 step. First step failed.
##############################################################

$UpdateVersion = '3.0'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."

    # --------------------------------------------------
    # Create a child update for all 'camera' component.
    # --------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-camera"
    $RefUpdateVersion = "1.1"
    $camerasFirmwareVersion = "1.1"

    Write-Host "    Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'cameras' group.
    $camerasSelector = @{ group = 'cameras' }
    $camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

    $cameraScriptFile = "$PSScriptRoot\scripts\contoso-camera-installscript.sh"
    $cameraFirmwareFile = "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #

    # This update contains 3 steps.
    $camerasInstallSteps = @()

    # Step #1 - simulating a failure while performing pre-install task.
    #           Note invalid HandlerProperties.scriptFileName value.
    #
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - pre-install step (failure - missing file)'

    # Step #2 - install a mock firmware onto camera component.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile, $cameraFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--firmware-file camera-firmware-$camerasFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - firmware installation'

    # Step #3 - simulating a success post-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                            }   `
                            `
                            -Description 'Cameras Update - post-install script'

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

# ------------------------------------------------------
# Create the parent update containing one reference step
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - A reference step for cameras update.
    $parentSteps += New-AduInstallationStep `
                        -UpdateId $camerasUpdateId `
                        -Description "Cameras Firmware Update"

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


##############################################################
# Update : 3.1
#
# Create the parent update containing 1 reference update
#     - Child update contains 3 steps. The second step failed.
##############################################################
$UpdateVersion = '3.1'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."

    # --------------------------------------------------
    # Create a child update for all 'camera' component.
    # --------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-camera"
    $RefUpdateVersion = "1.2"
    $camerasFirmwareVersion = "1.1"

    Write-Host "    Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'cameras' group.
    $camerasSelector = @{ group = 'cameras' }
    $camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

    $cameraScriptFile = "$PSScriptRoot\scripts\contoso-camera-installscript.sh"
    $cameraFirmwareFile = "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #
    # This update contains 3 steps.
    $camerasInstallSteps = @()

    # Step #1 - simulating a successful pre-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - pre-install step (failure - missing file)'

    # Step #2 - Simulating a failure while installing a mock firmware onto camera component.
    #           Note invalid HandlerProperties.scriptFileName value.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile, $cameraFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--firmware-file camera-firmware-$camerasFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - firmware installation'

    # Step #3 - simulating a success post-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                            }   `
                            `
                            -Description 'Cameras Update - post-install script'

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

# ------------------------------------------------------
# Create the parent update containing one reference step
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - A reference step for cameras update.
    $parentSteps += New-AduInstallationStep `
                        -UpdateId $camerasUpdateId `
                        -Description "Cameras Firmware Update"

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


##############################################################
# Update : 3.2
#
# Create the parent update containing 1 reference update
#     - Child update contains 3 steps. The third step failed.
##############################################################
$UpdateVersion = '3.2'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."

    # --------------------------------------------------
    # Create a child update for all 'camera' component.
    # --------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-camera"
    $RefUpdateVersion = "1.3"
    $camerasFirmwareVersion = "1.1"

    Write-Host "    Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'cameras' group.
    $camerasSelector = @{ group = 'cameras' }
    $camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

    $cameraScriptFile = "$PSScriptRoot\scripts\contoso-camera-installscript.sh"
    $cameraFirmwareFile = "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #
    # This update contains 3 steps.
    $camerasInstallSteps = @()

    # Step #1 - simulating a successful pre-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - pre-install step (failure - missing file)'

    # Step #2 - Simulating a successful firmware update.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile, $cameraFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--firmware-file camera-firmware-$camerasFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - firmware installation'

    # Step #3 - simulating a failed post-install task.
    #           Note invalid HandlerProperties.scriptFileName value.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                            }   `
                            `
                            -Description 'Cameras Update - post-install script'

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

# ------------------------------------------------------
# Create the parent update containing one reference step
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - A reference step for cameras update.
    $parentSteps += New-AduInstallationStep `
                        -UpdateId $camerasUpdateId `
                        -Description "Cameras Firmware Update"

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



##############################################################
# Update : 3.3
#
# Create the parent update containing 1 reference update
#     - Child update contains 3 steps. All steps succeess.
##############################################################
$UpdateVersion = '3.3'

$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

$RefUpdateNamePrefix = $UpdateName

Write-Host "Preparing update $parentUpdateIdStr ..."

    # --------------------------------------------------
    # Create a child update for all 'camera' component.
    # --------------------------------------------------

    $RefUpdateManufacturer = "contoso"
    $RefUpdateName = "$RefUpdateNamePrefix-virtual-camera"
    $RefUpdateVersion = "1.4"
    $camerasFirmwareVersion = "1.1"

    Write-Host "    Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

    $camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

    # This components update only apply to 'cameras' group.
    $camerasSelector = @{ group = 'cameras' }
    $camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

    $cameraScriptFile = "$PSScriptRoot\scripts\contoso-camera-installscript.sh"
    $cameraFirmwareFile = "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"

    #------------
    # ADD STEP(S)
    #
    # This update contains 3 steps.
    $camerasInstallSteps = @()

    # Step #1 - simulating a successful pre-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion";   `
                                'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - pre-install step (failure - missing file)'

    # Step #2 - Simulating a successful firmware update.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile, $cameraFirmwareFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                            }   `
                            `
                            -Description 'Cameras Update - firmware installation'

    # Step #3 - simulating a successful post-install task.
    $camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                            -Files  $cameraScriptFile `
                            -HandlerProperties @{  `
                                'scriptFileName'='contoso-camera-installscript.sh';  `
                                'installedCriteria'="$camerasFirmwareVersion"; `
                                "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                            }   `
                            `
                            -Description 'Cameras Update - post-install script'

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

# ------------------------------------------------------
# Create the parent update containing one reference step
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - A reference step for cameras update.
    $parentSteps += New-AduInstallationStep `
                        -UpdateId $camerasUpdateId `
                        -Description "Cameras Firmware Update"

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


