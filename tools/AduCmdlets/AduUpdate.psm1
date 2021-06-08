#
# Azure Device Update for IoT Hub
# Powershell module for creating import manifest and import API request body.
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0

# -------------------------------------------------
# CLASSES
# -------------------------------------------------

class CompatInfo
{
    [string] $DeviceManufacturer
    [string] $DeviceModel

    CompatInfo($manufacturer, $model)
    {
        $this.DeviceManufacturer = $manufacturer
        $this.DeviceModel = $model
    }
}

class UpdateId
{
    [string] $Provider
    [string] $Name
    [string] $Version

    UpdateId($provider, $name, $version)
    {
        $this.Provider = $provider
        $this.Name = $name
        $this.Version = $version
    }
}

# -------------------------------------------------
# INTERNAL METHODS
# -------------------------------------------------

function Get-UpdateId
{
    Param(
        [ValidateNotNullOrEmpty()]
        [string] $Provider,

        [ValidateNotNullOrEmpty()]
        [string] $Name,

        [ValidateNotNullOrEmpty()]
        [version] $Version
    )

    # Server will accept any order; preserving order for aesthetics only.
    $updateId = [ordered] @{
        'provider' = $Provider
        'name' = $Name
        'version' = "$Version"
    }

    return $updateId
}

function Get-FileMetadatas
{
    Param(
        [ValidateCount(0, 5)]
        [string[]] $FilePaths
    )

    $files = @()

    foreach ($filePath in $FilePaths)
    {
        if (!(Test-Path $filePath))
        {
            throw "$filePath could not be found."
        }

        $file = Get-Item $filePath
        $fileHashes = Get-AduFileHashes $filePath

        # Server will accept any order; preserving order for aesthetics only.
        $fileMap = [ordered] @{
            'filename' = $file.Name
            'sizeInBytes' = $file.Length
            'hashes' = $fileHashes
        }

        $files += $fileMap
    }

    return $files
}

# -------------------------------------------------
# EXPORTED METHODS
# -------------------------------------------------

function Get-AduFileHashes
{
<#
    .SYNOPSIS
        Get file hashes in a format required by ADU.

    .EXAMPLE
        PS C:\> Get-AduFileHashes -FilePath .\payload.bin
#>
    [CmdletBinding()]
    Param(
        # Full path to the file.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $FilePath
    )

    if (!(Test-Path $filePath))
    {
        throw "$FilePath could not be found."
    }

    $FilePath = Resolve-Path $FilePath

    $fs = [System.IO.File]::OpenRead($FilePath)
    $sha256 = New-Object System.Security.Cryptography.SHA256Managed
    $bytes = $sha256.ComputeHash($fs)
    $sha256.Dispose()
    $fs.Close()
    $fileHash = [System.Convert]::ToBase64String($bytes)

    $hashes = [pscustomobject]@{
        'sha256' = $fileHash
    }

    return $hashes
}

function New-AduUpdateId
{
<#
    .SYNOPSIS
        Create a new ADU update identity.

    .EXAMPLE
        PS C:\> New-AduUpdateId -Provider Contoso -Name Toaster -Version 1.0
#>
    [CmdletBinding()]
    Param(
        # Update provider.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $Provider,

        # Update name.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $Name,

        # Update version.
        [Parameter(Mandatory=$true)]
        [version] $Version
    )

    return [UpdateId]::New($Provider, $Name, $Version)
}

function New-AduUpdateCompatibility
{
<#
    .SYNOPSIS
        Create a new ADU update compatibility info.

    .EXAMPLE
        PS C:\> New-AduUpdateCompatibility -DeviceManufacturer Contoso -DeviceModel Toaster
#>
    [CmdletBinding()]
    Param(
        # Device manufacturer.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $DeviceManufacturer,

        # Device model.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $DeviceModel
    )

    return [CompatInfo]::New($DeviceManufacturer, $DeviceModel)
}

function New-AduImportManifest
{
<#
    .SYNOPSIS
        Create a new ADU update import manifest.

    .EXAMPLE
        PS C:\> $compatInfo1 = New-AduUpdateCompatibility -DeviceManufacturer Fabrikam -DeviceModel Toaster
        PS C:\> $compatInfo2 = New-AduUpdateCompatibility -DeviceManufacturer Contoso -DeviceModel Toaster
        PS C:\>
        PS C:\> New-AduImportManifest -Provider Fabrikam -Name Toaster -Version 2.0 `
                                      -UpdateType microsoft/swupdate:1 -InstalledCriteria 5 `
                                      -Compatibility $compatInfo1, $compatInfo2 `
                                      -Files '.\file1.json', '.\file2.zip'
#>
    [CmdletBinding()]
    Param(
        # Update identity created using New-AduUpdateId.
        [Parameter(ParameterSetName='UpdateId', Mandatory=$true)]
        [ValidateNotNull()]
        [UpdateId] $UpdateId,

        # Update provider.
        [Parameter(ParameterSetName='BackwardCompat', Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $Provider,

        # Update name.
        [Parameter(ParameterSetName='BackwardCompat', Mandatory=$true)]
        [ValidateLength(1, 64)]
        [ValidatePattern("^[a-zA-Z0-9.-]+$")]
        [string] $Name,

        # Update version.
        [Parameter(ParameterSetName='BackwardCompat', Mandatory=$true)]
        [version] $Version,

        # Update type in form of "{provider}/{updateType}:{updateTypeVersion}"
        # This parameter is forwarded to client device during deployment. It may be used to determine whether this update is of supported types.
        # For example, reference ADU agent uses the following:
        # - "microsoft/swupdate:1" for SwUpdate image-based update.
        # - "microsoft/apt:1" for APT package-based update.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 32)]
        [ValidatePattern("^\S+\/\S+:\d{1,5}$")]
        [string] $UpdateType,

        # Criteria for update to be considered installed.
        # This parameter is forwarded to client device during deployment.
        # For example, reference ADU agent expects the following value:
        # - Value of SwVersion for SwUpdate image-based update.
        # - '{name}-{version}' of which name and version are obtained from the APT file.
        [Parameter(Mandatory=$true)]
        [ValidateLength(1, 64)]
        [string] $InstalledCriteria,

        # List of compatibility information of devices this update is compatible with, created using New-AduUpdateCompatibility.
        [Parameter(Mandatory=$true)]
        [ValidateCount(1, 10)]
        [CompatInfo[]] $Compatibility,

        # List of full paths to update files.
        [ValidateCount(0, 5)]
        [string[]] $Files = @(),

        # List of bundled updateIds.
        [ValidateCount(0, 10)]
        [UpdateId[]] $BundledUpdates = @()
    )

    if (($null -eq $Files -or $Files.Length -eq 0) -and ($null -eq $BundledUpdates -or $BundledUpdates.Length -eq 0))
    {
        # only bundle update is allowed to have no payload file.
        Write-Error "Update must contain at least one file."
    }

    if ($null -ne $UpdateId)
    {
        $id = Get-UpdateId -Provider $UpdateId.Provider -Name $UpdateId.Name -Version $UpdateId.Version
    }
    else
    {
        $id = Get-UpdateId -Provider $Provider -Name $Name -Version $Version
    }

    $fileMetadata = Get-FileMetadatas $Files
    $createdDate = (Get-Date).ToUniversalTime().ToString('o') # ISO8601

    # Server will accept any order; preserving order for aesthetics only.
    $importManifest = [ordered] @{
        'updateId' = $id
        'updateType' = $UpdateType
        'installedCriteria' = $InstalledCriteria
        'compatibility' = [array] $Compatibility
        'createdDateTime' = $createdDate
        'manifestVersion' = '3.0'
    }

    if ($fileMetadata.Length -gt 0)
    {
        $importManifest['files'] = [array] $fileMetadata
    }

    if ($BundledUpdates.Length -gt 0)
    {
        $importManifest['bundledUpdates'] = [array] $BundledUpdates
    }

    ConvertTo-Json -InputObject $importManifest -Depth 10
}

Export-ModuleMember -Function New-AduImportManifest, New-AduUpdateId, New-AduUpdateCompatibility, Get-AduFileHashes
