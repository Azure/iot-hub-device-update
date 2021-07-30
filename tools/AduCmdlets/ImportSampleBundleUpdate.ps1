#
# Device Update for IoT Hub
# Sample PowerShell script for creating and importing bundle update.
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Sample script for creating a sample bundle update with single leaf (bundled) update, and importing it to
        Azure Device Update for IoT Hub through REST API. The sample update will not be installable on any device.

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $container = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $containerName
        PS >
        PS > $token = Get-MsalToken -ClientId $clientId -TenantId $tenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$tenantId/v2.0 -Interactive -DeviceCode
        PS >
        PS > ImportSampleBundleUpdate.ps1 -AccountEndpoint sampleaccount.api.adu.microsoft.com -InstanceId sampleinstance `
                                          -Container $container `
                                          -AuthorizationToken $token
#>
[CmdletBinding()]
Param(
    # ADU account endpoint, e.g. sampleaccount.api.adu.microsoft.com
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AccountEndpoint,

    # ADU Instance Id.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $InstanceId,

    # Azure Storage Blob container for staging update artifacts.
    [Parameter(Mandatory=$true)]
    [ValidateNotNull()]
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $Container,

    # Azure Active Directory OAuth authorization token.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AuthorizationToken,

    # Version of update to create and import.
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion = '1.0'
)

Import-Module $PSScriptRoot\AduImportUpdate.psm1 -ErrorAction Stop
Import-Module $PSScriptRoot\AduRestApi.psm1 -ErrorAction Stop

# ------------------------------
# Create a leaf (bundled) update
# ------------------------------
Write-Host 'Preparing leaf update ...'

# We will use a random file as the update payload file.
$file = "$PSScriptRoot\README.md"

$leafUpdateId = New-AduUpdateId -Provider Microsoft -Name Microphone -Version $UpdateVersion
$leafCompat = New-AduUpdateCompatibility -DeviceManufacturer Microsoft -DeviceModel Microphone
$leafUpdate = New-AduImportUpdateInput -UpdateId $leafUpdateId `
                                       -IsDeployable $false `
                                       -UpdateType 'microsoft/component:1' -InstalledCriteria 'v1' -Compatibility $leafCompat `
                                       -Files $file `
                                       -BlobContainer $Container -ErrorAction Stop -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Create the bundle update which bundles the leaf update above
# ------------------------------------------------------------
Write-Host 'Preparing bundle update ...'

$bundleUpdateId = New-AduUpdateId -Provider Microsoft -Name Surface -Version $UpdateVersion
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
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 10) -Verbose:$VerbosePreference