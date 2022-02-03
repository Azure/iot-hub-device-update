#
# Device Update for IoT Hub
# Sample PowerShell script for creating and importing updates with multiple inline steps.
# Some update demonstrates error scenarios where payload file is invalid.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Create a sample update with multiple inline steps, and importing it to Device Update for IoT Hub
        through REST API. This sample update contain fake files and cannot be actually installed onto a real device.

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $BlobContainer = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $BlobContainerName
        PS >
        PS > $token = Get-MsalToken -ClientId $clientId -TenantId $tenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$tenantId/v2.0 -Interactive -DeviceCode
        PS >
        PS > ImportSampleMSOEUpdate-2.x.ps1 -AccountEndpoint sampleaccount.api.adu.microsoft.com -InstanceId sampleinstance `
                                            -Container $BlobContainer `
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
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $BlobContainer,

    # Azure Active Directory OAuth authorization token.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AuthorizationToken,

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = "ADU-Client-Eng",

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = "MSOE-Update-Demo",
    
    # Device Manufacturer
    [ValidateNotNullOrEmpty()]
    [string] $DeviceManufacturer = "adu-device",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $DeviceModel = "e2e-test"
)

Import-Module $PSScriptRoot\AduImportUpdate.psm1 -ErrorAction Stop
Import-Module $PSScriptRoot\AduRestApi.psm1 -ErrorAction Stop

$UpdateVersion = '2.0'

# ------------------------------------------------------------
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
Write-Host 'Preparing parent update 2.0 ...'

$parentFile0 = "$PSScriptRoot\data-files\APT\apt-manifest-1.0.json"
$parentFile1 = "$PSScriptRoot\data-files\APT\apt-manifest-tree-1.0.json"
$parentFileBad = "$PSScriptRoot\data-files\APT\bad-apt-manifest-1.0.json"

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()

# Step #1 - Simulating an inline step that contains bad payload file (invalid APT manifest)
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFileBad -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.0'} -Description 'Install APT update with bad manifest file (failure case)' 

# Step #2 - An inline step that contains valid payload file
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFile1 -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.0'} -Description 'Install tree package on host device'

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
$UpdateVersion = '2.1'
Write-Host 'Preparing parent update 2.1 ...'

$parentFile0 = "$PSScriptRoot\data-files\APT\apt-manifest-1.0.json"
$parentFile1 = "$PSScriptRoot\data-files\APT\apt-manifest-tree-1.0.json"
$parentFileBad = "$PSScriptRoot\data-files\APT\bad-apt-manifest-1.0.json"

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()

# Step #1 - An inline step that contains valid payload file
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFile0 -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.1'} -Description 'Install libcurl4-doc on host device'

# Step #2 - Simulating an inline step that contains bad payload file (invalid APT manifest)
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFileBad -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.1'} -Description 'Install APT update with bad manifest file (failure case)'

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Create the parent update containing 2 inline steps
# ------------------------------------------------------------
$UpdateVersion = '2.2'
Write-Host 'Preparing parent update 2.2 ...'

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()

# Step #1 - An inline step that contains valid payload file
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFile0 -HandlerProperties @{ 'installedCriteria'='apt-update-test-2.2'} -Description 'Install libcurl4-doc on host device'

# Step #2 - An inline step that contains valid payload file
$parentSteps += New-AduInstallationStep -Handler 'microsoft/apt:1' -Files $parentFile1 -HandlerProperties @{ 'installedCriteria'='apt-update-test-tree-2.2'} -Description 'Install tree on host device'

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference
