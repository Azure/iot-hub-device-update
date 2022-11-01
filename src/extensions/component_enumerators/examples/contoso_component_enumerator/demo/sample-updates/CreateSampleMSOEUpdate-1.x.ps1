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
        PS > CreateSampleMSOEUpdate-1.x.ps1 -UpdateProvider "Contoso" `
                                             -UpdateName "Virtual-Vacuum" `
                                             -Manufacturer "contoso" `
                                             -Model "virtual-vacuum-v1" `
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
    [string] $Manufacturer = "contoso",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $Model = "virtual-vacuum-v1"
)

# ------------------------------------------------------------
# Update : 0.1
# Create the parent update with bad APT Manifest file.
# ------------------------------------------------------------
$UpdateVersion = '0.1'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"
$parentSteps = @()

Write-Host "Preparing parent update $UpdateVersion ..."

$parentFile0 = "$PSScriptRoot\data-files\APT\bad-apt-manifest-1.0.json"


    # -----------
    # ADD STEP(S)

    # Step 0 - Simulating a failure during an install task (for testing purposes)
    #          by intentionally using mal-formed APT manifest file.
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFile0 `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-1.0'} `
                        -Description 'Example APT update (fail) with bad APT manifest file'

# ------------------------------
# Create parent update manifest
Write-Host "    Generating an import manifest $parentUpdateIdStr..."
$payloadFiles = $parentFile0

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
Copy-Item -Path $payloadFiles -Destination $outputPath -Force

Write-Host " "


# ---------------------------------------------------------------
# Update 1.0
#
# Create the parent update with good APT Manifest file.
# This update installs libcurl4-doc package on the target device.
# ---------------------------------------------------------------
$UpdateVersion = '1.0'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"
$parentSteps = @()

Write-Host "Preparing parent update $UpdateVersion ..."

$parentFile1 = "$PSScriptRoot\data-files\APT\apt-manifest-1.0.json"

    # -----------
    # ADD STEP(S)

    # Step 0 - APT update that installs 'tree'' debian package.
    $parentSteps += New-AduInstallationStep `
                            -Handler 'microsoft/apt:1' `
                            -Files $parentFile1 `
                            -HandlerProperties @{ 'installedCriteria'='apt-update-test-1.1'} `
                            -Description 'APT update that installs libcurl4-doc package.'

# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."
$payloadFiles = $parentFile1

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
Copy-Item -Path $payloadFiles -Destination $outputPath -Force

Write-Host " "
