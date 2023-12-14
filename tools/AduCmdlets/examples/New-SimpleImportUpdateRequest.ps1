#
# Device Update for IoT Hub
# Sample PowerShell script for preparing Import Update API request from a simple import manifest (no reference steps).
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Create import update API request body from a simple import manifest (no reference steps).

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $BlobContainer = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $BlobContainerName
        PS >
        PS > New-SimpleImportUpdateRequest.ps1 -ImportManifestJson .\importmanifest.json - PayloadFiles file01.bin, file02.bin -Container $BlobContainer
#>
[CmdletBinding()]
Param(
    # Import manifest JSON file.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrWhiteSpace()]
    [string] $ImportManifestJson,

    # Update payload files.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrWhiteSpace()]
    [string[]] $PayloadFiles,

    # Azure Storage Blob container for staging update artifacts.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrWhiteSpace()]
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $BlobContainer
)

Import-Module $PSScriptRoot\..\AduAzStorageBlobHelper.psm1 -Force -Scope Local -ErrorAction Stop
Import-Module $PSScriptRoot\..\AduUpdate.psm1 -Force -Scope Local -ErrorAction Stop

$importMan = Get-Content $ImportManifestJson | ConvertFrom-Json -Depth 20
$updateIdStr = "$($importMan.UpdateId.Provider).$($importMan.UpdateId.Name).$($importMan.UpdateId.Version)"

$importManJsonFile = Get-Item $ImportManifestJson
$importManJsonHash = Get-AduFileHashes -FilePath $ImportManifestJson -ErrorAction Stop
$importManUrl = Copy-AduFileToAzBlobContainer -FilePath $ImportManifestJson -BlobName "$updateIdStr/importmanifest.json" -BlobContainer $BlobContainer -ErrorAction Stop

$fileMetaList = @()

$PayloadFiles | ForEach-Object {
    # Place files within updateId subdirectory in case there are filenames conflict.
    $filename = Split-Path -Leaf (Resolve-Path $_)
    $blobName = "$updateIdStr/$filename"

    $url = Copy-AduFileToAzBlobContainer -FilePath $_ -BlobName $blobName -BlobContainer $BlobContainer -ErrorAction Stop

    $fileMeta = New-Object PSObject | Select-Object filename, url
    $fileMeta.filename = $filename
    $fileMeta.url = $url

    $fileMetaList += $fileMeta
}

$importManMeta = [ordered] @{
    url = $importManUrl
    sizeInBytes = $importManJsonFile.Length
    hashes = $importManJsonHash
}

$importUpdateInput = [ordered] @{
    importManifest = $importManMeta
    files = $fileMetaList
}

return @($importUpdateInput)
