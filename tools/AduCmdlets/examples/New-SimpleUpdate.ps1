#
# Device Update for IoT Hub
# Sample PowerShell script for creating a simple update (no reference steps).
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0

<#
    .SYNOPSIS
        Create a sample update with all inline steps. This sample update contain fake files and cannot be actually installed onto a device.

    .EXAMPLE
        PS > New-SimpleUpdate.ps1 -Path ./test -UpdateVersion 1.2
#>
[CmdletBinding()]
Param(
    # Directory to create the import manifest JSON files in.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $Path,

    # Version of update to create and import. E.g. 1.0
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion
)

Import-Module $PSScriptRoot/../AduUpdate.psm1 -Force -Scope Local -ErrorAction Stop

# We will use arbitrary files as update payload files.
$payloadFile = "$env:TEMP/payloadFile.bin.txt"
"This is an update payload file." | Out-File $payloadFile -Force -Encoding utf8

# ------------------------------
# Create a child update
# ------------------------------
Write-Host 'Preparing child update ...'

$toasterUpdateId = New-AduUpdateId -Provider Contoso -Name Toaster -Version $UpdateVersion
$toasterCompat = New-AduUpdateCompatibility -Manufacturer Contoso -Model Toaster
$toasterInstallStep = New-AduInstallationStep -Handler 'microsoft/swupdate:1' -Files $payloadFile
$toasterUpdate = New-AduImportManifest -UpdateId $toasterUpdateId `
                                             -IsDeployable $true `
                                             -Compatibility $toasterCompat `
                                             -InstallationSteps $toasterInstallStep `
                                             -ErrorAction Stop -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Write all to files
# ------------------------------------------------------------
Write-Host 'Saving manifest and update files ...'

New-Item $Path -ItemType Directory -Force | Out-Null

$toasterUpdate | Out-File "$Path/$($toasterUpdateId.Provider).$($toasterUpdateId.Name).$($toasterUpdateId.Version).importmanifest.json" -Encoding utf8

Copy-Item $payloadFile -Destination $Path -Force

Write-Host "Import manifest JSON files saved to $Path" -ForegroundColor Green

Remove-Item $payloadFile -Force -ErrorAction SilentlyContinue | Out-Null
