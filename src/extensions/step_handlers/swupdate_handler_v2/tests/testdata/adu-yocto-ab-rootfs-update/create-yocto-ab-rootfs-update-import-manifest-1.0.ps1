#
# Device Update for IoT Hub
# Sample PowerShell script for creating import manifests and payloads files for demo purposes.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create an import manifest (v4) for A/B root file system update.

    .EXAMPLE
        PS > create-yocto-ab-rootfs-update-import-manifest.ps1 `
                -UpdateProvider "Contoso" `
                -UpdateName "Virtual-Vacuum" `
                -Manufacturer "contoso" `
                -Model "virtual-vacuum-v1" `
                -UpdateVersion "1.0" `
                -SwuFileName "my-update.swu" `
                -InstalledCriteria "8.0.1.0001" `
                -OutputManifestPath .
#>
[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputManifestPath,

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = "EDS-ADUClient",

    # Update Name
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = "yocto-update",

    # Device Manufacturer
    [ValidateNotNullOrEmpty()]
    [string] $Manufacturer = "contoso",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $Model = "virtual-vacuum-v2",

    # Update Version
    # NOTE: This is the version of the update. Not the version of the image in .swu file.
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion = "1.0",

    # Update Filename
    [ValidateNotNullOrEmpty()]
    [string] $SwuFileName = "my-update.swu",

    # Installed Criteria
    # NOTE: This is the version number in the '--software-version-file'
    [ValidateNotNullOrEmpty()]
    [string] $InstalledCriteria = "8.0.1.0001"
)

# ------------------------------------------------------------
# Create the parent update.
# ------------------------------------------------------------

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"
$parentSteps = @()


# Payloads for top-level inline steps

# Source files
$PayloadsDir = "payloads"

$parentFile0 = "$PSScriptRoot\$PayloadsDir\example-a-b-update.sh"
$parentFile1 = "$PSScriptRoot\$PayloadsDir\$SwuFileName"

$payloadFiles = $parentFile0, $parentFile1

Write-Host "#"
Write-Host "# Generating an import manifest"
Write-Host "#"
Write-Host "# Udate version : $UpdateVersion"
Write-Host "#"
Write-Host "# Payloads :"
Write-Host "#    $parentFile0"
Write-Host "#    $parentFile1"

    # -----------
    # ADD TOP-LEVEL STEP(S)

    # ======
    # Step 1
    # ======
    # This step demonstrates software installation using 'swupdate' program.
    # The step handler type is 'microsoft/swupdate:2' which corresponding to SWUpdate Handler v2.
    #
    # The SWUpdate Handler v2 requires following handlerProperties:
    #
    #   swuFile           - Name of the image (.swu) file to be installed on the target device.
    #
    #   scriptFile        - Name of the primary script file that implements download, install, apply, cancel and isInstalled logic.
    #                       This update uses 'example-a-b-update.sh' script.
    #                       The script supports following actions:
    #                            --action-download     : No-op. (No additional payloads)
    #                            --action-install      : Invokes swupdate to install the specified 'swuFile'
    #                            --action-apply        : Sets uboot environment variable to switch to the desired rootfs partition after rebooted.
    #                                                    Returns a special (ADUC_Result) result code to instruct DU Agent to reboot the device.
    #                            --action-cancel       : Restore uboot environment variable to the value before '--action-apply' is performed.
    #                            --action-is-installed : Returns (ADUC_Result) result code that indicates whether the current rootfs contains
    #                                                    software (image) version that match specified 'installCriteria' string.
    #
    #   installedCriteria - String used to evaluate whether the software is installed on the target.
    #                       When performing '--action-apply', the primary script file compare this value against content of the
    #                       specified '--software-version-file' in 'arguments' property (see below).
    #
    #   arguments         - List of options and arguments that will be passed to the primary script.
    #
    #                       Required argument:
    #                           --software-version-file <filepath>
    #                                 A full path to a file (on a target device) containing a single line of text that identifies the image version.
    #                                 Default path '/etc/adu-version
    #
    #                       Optional argument:
    #                           --swupdate-log-file <filepath>
    #                                 A full path to a file that swupdate will write an output into.
    #                                 Default path '/adu/logs/swupdate.log'
    #
    $parentSteps += New-AduInstallationStep `
                        -Handler 'microsoft/swupdate:2'     `
                        -Files $payloadFiles  `
                        -HandlerProperties @{               `
                            'scriptFileName'='example-a-b-update.sh'; `
                            'swuFileName'="$SwuFileName"; `
                            'installedCriteria'="$InstalledCriteria";   `
                            'arguments'='--software-version-file /etc/adu-version --swupdate-log-file /adu/logs/swupdate.log'
                        } `
                        -Description 'Update rootfs using A/B update strategy'

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
