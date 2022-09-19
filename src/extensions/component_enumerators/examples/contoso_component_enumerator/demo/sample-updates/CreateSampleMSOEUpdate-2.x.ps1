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
        PS > CreateSampleMSOEUpdate-2.x.ps1 -UpdateProvider "Contoso" `
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
# Update : 2.0
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
$UpdateVersion = '2.0'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

Write-Host "Preparing parent update $UpdateVersion ..."

$parentFile0 = "$PSScriptRoot\data-files\APT\apt-manifest-1.0.json"
$parentFile1 = "$PSScriptRoot\data-files\APT\apt-manifest-tree-1.0.json"
$parentFileBad = "$PSScriptRoot\data-files\APT\bad-apt-manifest-1.0.json"

    # -----------
    # ADD STEP(S)
    $parentSteps = @()

    # Step #1 - Simulating an inline step that contains bad payload file (invalid APT manifest)
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFileBad `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.0'} `
                        -Description 'Example APT update (fail) with bad APT manifest file'

    # Step #2 - An inline step that contains valid payload file
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFile1 `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.0'} `
                        -Description 'Install tree debian package on host device'

# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."
$payloadFiles = $parentFileBad, $parentFile1

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
# Update 2.1
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
$UpdateVersion = '2.1'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"

Write-Host "Preparing parent update $UpdateVersion ..."

$parentFile0 = "$PSScriptRoot\data-files\APT\apt-manifest-1.0.json"
$parentFile1 = "$PSScriptRoot\data-files\APT\apt-manifest-tree-1.0.json"
$parentFileBad = "$PSScriptRoot\data-files\APT\bad-apt-manifest-1.0.json"

    # -----------
    # ADD STEP(S)
    $parentSteps = @()

    # Step #1 - An inline step that contains valid payload file
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFile0 `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.1'} `
                        -Description 'Install libcurl4-doc on host device'

    # Step #2 - Simulating an inline step that contains bad payload file (invalid APT manifest)
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFileBad `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.1'} `
                        -Description 'Install APT update with bad manifest file (failure case)'

# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."
$payloadFiles = $parentFile0, $parentFileBad

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
# Update 2.2
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
$UpdateVersion = '2.2'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"
$parentSteps = @()

Write-Host "Preparing parent update $UpdateVersion ..."

    # -----------
    # ADD STEP(S)

    # Step #1 - An inline step that contains valid payload file
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFile0 `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.2'} `
                        -Description 'Install libcurl4-doc on host device'

    # Step #2 - An inline step that contains valid payload file
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files $parentFile1 `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.2'} `
                        -Description 'Install tree on host device'


# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."
$payloadFiles = $parentFile0, $parentFile1

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
