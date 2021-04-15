# AduUpdate.psm1 PowerShell Script

## Overview

PowerShell script module `AduUpdate.psm1` can be used to create an *import manifest* to import an update into Device Update for IoT Hub. It requires PowerShell 5 or later.

## Usage

To create an import manifest:

```powershell
Import-Module .\AduUpdate.psm1

# Create an update that will be compatible with devices from two different manufacturers.
$compatInfo1 = New-AduUpdateCompatibility `
    -DeviceManufacturer Fabrikam `
    -DeviceModel Toaster
$compatInfo2 = New-AduUpdateCompatibility `
    -DeviceManufacturer Contoso `
    -DeviceModel Toaster

$importManifest = New-AduImportManifest `
    -Provider 'Microsoft' `
    -Name 'Toaster' `
    -Version '2.0' `
    -UpdateType 'microsoft/swupdate:1' `
    -InstalledCriteria '5' `
    -Compatibility $compatInfo1, $compatInfo2 `
    -Files '.\file1.json', '.\file2.zip'

$importManifestFile = '.\importManifest.json'
$importManifest | Out-File $importManifestFile -Encoding UTF8
```

The sample commands above will produce the following import manifest (note that **sizeInbytes** and **sha256** will vary based on the actual files used):

```json
{
  "updateId": {
    "provider": "Microsoft",
    "name": "Toaster",
    "version": "2.0"
  },
  "updateType": "microsoft/swupdate:1",
  "installedCriteria": "5",
  "compatibility": [
    {
      "deviceManufacturer": "Fabrikam",
      "deviceModel": "Toaster"
    },
    {
      "deviceManufacturer": "Contoso",
      "deviceModel": "Toaster"
    }
  ],
  "files": [
    {
      "filename": "file1.json",
      "sizeInBytes": 7,
      "hashes": {
        "sha256": "K2mn97qWmKSaSaM9SFdhC0QIEJ/wluXV7CoTlM8zMUo="
      }
    },
    {
      "filename": "file2.zip",
      "sizeInBytes": 11,
      "hashes": {
        "sha256": "gbG9pxCr9RMH2Pv57vBxKjm89uhUstD06wvQSioLMgU="
      }
    }
  ],
  "createdDateTime": "2020-10-08T03:32:52.477Z",
  "manifestVersion": "2.0"
}
```

Note: `AduUpdate.psm1` also exports a method that can be used to calculate hashes of import manifest file, which will be needed if calling `Import Update` REST API directly:

```powershell
Get-AduFileHashes -FilePath $importManifestFile
```

Output:

```PowerShell
sha256
------
AM5H3a8hp4PPSFRMYoqgzXZegaISpfn6psVoUFs+UIA=
```

## Documentation

To get detailed help information:

```powershell
Get-Help New-AduUpdateCompatibility -Detailed
Get-Help New-AduImportManifest -Detailed
Get-Help Get-AduFileHashes -Detailed
```
