#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Sample script for creating a sample bundle update with single leaf (bundled) update, and importing it to
        Azure Device Update for IoT Hub through REST API. The sample update will not be installable on any device.
#>
[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AccountEndpoint,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $InstanceId,

    [Parameter(Mandatory=$true)]
    [ValidateNotNull()]
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $Container,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AuthorizationToken
)

Import-Module $PSScriptRoot\AduImportUpdate.psm1 -ErrorAction Stop
Import-Module $PSScriptRoot\AduRestApi.psm1 -ErrorAction Stop

# ------------------------------
# Create a leaf (bundled) update
# ------------------------------
Write-Host 'Preparing leaf update ...'

# We will use a random file as the update payload file.
$file = "$PSScriptRoot\README.md"

$leafUpdateId = New-AduUpdateId -Provider Microsoft -Name Microphone -Version 1.0
$leafCompat = New-AduUpdateCompatibility -DeviceManufacturer Microsoft -DeviceModel Microphone
$leafUpdate = New-AduImportUpdateInput -UpdateId $leafUpdateId `
                                       -UpdateType 'microsoft/component:1' -InstalledCriteria 'v1' -Compatibility $leafCompat `
                                       -Files $file `
                                       -BlobContainer $Container -ErrorAction Stop -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Create the bundle update which bundles the leaf update above
# ------------------------------------------------------------
Write-Host 'Preparing bundle update ...'

$bundleUpdateId = New-AduUpdateId -Provider Microsoft -Name Surface -Version 1.0
$bundleCompat = New-AduUpdateCompatibility -DeviceManufacturer Microsoft -DeviceModel Surface
$bundleUpdate = New-AduImportUpdateInput -UpdateId $bundleUpdateId `
                                         -UpdateType 'microsoft/bundle:1' -InstalledCriteria 'v1' -Compatibility $bundleCompat `
                                         -BundledUpdates $leafUpdateId `
                                         -BlobContainer $Container -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $bundleUpdate, $leafUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Verbose:$VerbosePreference