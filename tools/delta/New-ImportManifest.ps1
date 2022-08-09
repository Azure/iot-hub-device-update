<#
.SYNOPSIS
Commandlet to create ADU import manifest file.

.DESCRIPTION
Use this commandlet to generate ADU import manifest file content.

.EXAMPLE
PS> New-ImportManifest.ps1 `
      -UpdateProvider "provider" -UpdateName "name" -UpdateVersion "2.0.0.0" `
      -CompatibilityDeviceManufacturer "manufacturer" -CompatibilityDeviceModel "model" `
      -SwuInstalledCriteria "2.0.0.0" `
      -File .\myPayloadFileV2.swu `
      -DeltaFile .\myDeltaFileV1toV2.dat `
      -SourceFile .\myPayloadFileV1.swu 
Create import manifest to import payload file with specified delta file as a related file.

.EXAMPLE
PS> New-ImportManifest.ps1 `
      -UpdateProvider "provider" -UpdateName "name" -UpdateVersion "1.0.0.0" `
      -CompatibilityDeviceManufacturer "manufacturer" -CompatibilityDeviceModel "model" `
      -SwuInstalledCriteria "1.0.0.0" `
      -File .\myPayloadFileV1.swu `
      | Out-File .\import-manifest.json
Create import manifest and save it into a local file named "import-manifest.json" in the current folder.
#>

Param(
    ## Update identity provider.
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = $(throw "'UpdateProvider' parameter is required."),

    ## Update identity name.
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = $(throw "'UpdateName' parameter is required."),

    ## Update identity version.
    [ValidateNotNullOrEmpty()]
    [version] $UpdateVersion = $(throw "'UpdateVersion' parameter is required."),

    ## Device manufacturer compatibility.
    [ValidateNotNullOrEmpty()]
    [string] $CompatibilityDeviceManufacturer = $(throw "'CompatibilityDeviceManufacturer' parameter is required."),

    ## Device model compatibility.
    [ValidateNotNullOrEmpty()]
    [string] $CompatibilityDeviceModel = $(throw "'CompatibilityDeviceModel' parameter is required."),

    ## SWU handler installed criteria.
    [ValidateNotNullOrEmpty()]
    [string] $SwuInstalledCriteria = $(throw "'SwuInstalledCriteria' parameter is required."),

    ## Payload file.
    [ValidateNotNullOrEmpty()]
    [string] $File = $(throw "'File' parameter is required."),

    ## Source file used to generate DeltaFile.
    [string] $SourceFile,

    ## Delta file.
    [string] $DeltaFile
)

# Local functions
    function Get-FileMetadata([string] $filePath)
    {
        if (!(Test-Path $filePath))
        {
            throw "$filePath could not be found."
        }

        $file = Get-Item $filePath
        $fileHashes = Get-FileHashes $filePath

        # Server will accept any order; preserving order for aesthetics only.
        $fileMetadata = [ordered] @{
            'filename' = $file.Name
            'sizeInBytes' = $file.Length
            'hashes' = $fileHashes
        }

        return $fileMetadata
    }

    function Get-FileHashes([string] $filePath)
    {
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

# Main
    #Process file
    $payloadMetadata = Get-FileMetadata $file

    $importManifest = [ordered] @{
      updateId = [ordered] @{
        provider = $UpdateProvider
        name = $UpdateName
        version = $UpdateVersion.ToString()
      }

      compatibility = @( 
        @{
          deviceManufacturer = $CompatibilityDeviceManufacturer
          deviceModel = $CompatibilityDeviceModel
        })

      instructions = @{
        steps = @(
          [ordered] @{
            type = "inline"
            handler = "microsoft/swupdate:1"
            files = @( $payloadMetadata.filename )
            handlerProperties = [ordered] @{
              installedCriteria = $SwuInstalledCriteria
            }
          })
      }

      files = @()
    }

    if (!([string]::IsNullOrWhitespace($DeltaFile)) -and (Test-Path $DeltaFile))
    {
        if (([string]::IsNullOrWhitespace($SourceFile)) -or !(Test-Path $SourceFile))
        {
          throw "If you specify 'DeltaFile' argument, then you have to provide 'SourceFile' argument as well."
        }

        $sourceMetadata = Get-FileMetadata $SourceFile
        $deltaMetadata = Get-FileMetadata $DeltaFile
        $deltaMetadata.properties = [ordered] @{
          "microsoft.sourceFileHashAlgorithm" = "sha256"
          "microsoft.sourceFileHash" = $sourceMetadata.hashes.sha256
        }

        $payloadMetadata.relatedFiles = @( $deltaMetadata )
        $payloadMetadata.downloadHandler = [ordered] @{
          id = "microsoft/delta:1"
        }
    }

    $importManifest.files += $payloadMetadata

    $importManifest.createdDateTime = (Get-Date).ToUniversalTime().ToString('o') # ISO8601
    $importManifest.manifestVersion = '5.0'

    ConvertTo-Json -InputObject $importManifest -Depth 20