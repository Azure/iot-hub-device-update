#
# Device Update for IoT Hub
# Sample PowerShell script for creating import manifests and payloads files for demomstration purposes.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create an import manifest (v4) for A/B root file system update.

    .EXAMPLE
        PS > create-script-update-import-manifest.ps1 `
                -UpdateProvider "fabrikam" `
                -UpdateName "rpi-ubuntu-2004-adu-update" `
                -Manufacturer "contoso" `
                -Model "rpi-ubuntu-2004-adu" `
                -UpdateVersion "1.0.1" `
                -OutputManifestPath .
#>
[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputManifestPath,

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = "fabrikam",

    # Update Name
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = "rpi-ubuntu-2004-adu-update",

    # Device Manufacturer
    [ValidateNotNullOrEmpty()]
    [string] $Manufacturer = "contoso",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $Model = "rpi-ubuntu-2004-adu",

    # Update Version
    # NOTE: This is the version of the update. Not the version of the image in .swu file.
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion = "1.0.1"
)

# ------------------------------------------------------------
# Create the parent update.
# ------------------------------------------------------------

# Auto generate installed criteria string.
$InstalledCriteria = "ubuntu-20.04-adu-$UpdateVersion"

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"
$parentSteps = @()


$DefaultScriptFileName="simple-a-b-update-script.sh"

# Payloads for top-level inline steps

# Source files
$PayloadsDir = "payloads"

$parentFile0 = "$PSScriptRoot\$PayloadsDir\$DefaultScriptFileName"

$payloadFiles = $parentFile0

Write-Host "#"
Write-Host "# Generating an import manifest"
Write-Host "#"
Write-Host "# Udate version : $UpdateVersion"
Write-Host "#"
Write-Host "# Payloads :"
Write-Host "#    $parentFile0"

    # -----------
    # ADD TOP-LEVEL STEP(S)

    # ======
    # Step 1
    # ======
    # This step demonstrates software installation using script handler.
    # The step handler type is 'microsoft/script:1' which corresponding to Script Handler.
    #
    #   scriptFileName    - Name of the primary script file that implements download, install, apply, cancel and isInstalled logic.
    #                       This update uses 'simple-a-b-update-script.sh' script.
    #                       The script supports following actions:
    #                            --action-download     : No-op. (No additional payloads)
    #                            --action-install      : Invokes swupdate to install the specified 'swuFile'
    #                            --action-apply        : Sets uboot environment variable to switch to the desired rootfs partition after rebooted.
    #                                                    Returns a special (ADUC_Result) result code to instruct DU Agent to reboot the device.
    #                            --action-cancel       : Restore uboot environment variable to the value before '--action-apply' is performed.
    #                            --action-is-installed : Returns (ADUC_Result) result code that indicates whether the current rootfs contains
    #                                                    software (image) version that match specified 'installCriteria' string.
    #
    #
    #   installedCriteria - String used to evaluate whether the software is installed on the target.
    #                       When performing '--action-apply', the primary script file compare this value against content of the
    #                       specified '--software-version-file' in 'arguments' property (see below).
    #
    #
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/script:1'     `
                        -Files $payloadFiles  `
                        -HandlerProperties @{ `
                            'scriptFileName'="$DefaultScriptFileName"; `
                            'installedCriteria'="$InstalledCriteria"; `
                        } `
                        -Description 'Update the Root FS using A/B update strategy.'

# ------------------------------
# Create parent update manifest
Write-Host "#"
Write-Host "#    Output directory: $parentUpdateIdStr"

$parentManifest = New-AduImportManifest -UpdateId $parentUpdateId `
                                    -IsDeployable $true `
                                    -Compatibility $parentCompat `
                                    -InstallationSteps $parentSteps `
                                    -ErrorAction Stop

# Create folder for manifest files and payload.
$outputPath = "$OutputManifestPath\$parentUpdateIdStr"
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest file.
$outputManifestFile = "$outputPath\$parentUpdateIdStr.importmanifest.json"
Write-Host "#"
Write-Host "#    Import manifest: $outputManifestFile"
$parentManifest | Out-File "$outputManifestFile" -Encoding utf8

# Copy all payloads (if used)
if ($payloadFiles) { `
    Write-Host "#    Saved payload(s) to: $outputPath"
    Copy-Item -Path $payloadFiles -Destination $outputPath -Force `
}

Write-Host "#"
