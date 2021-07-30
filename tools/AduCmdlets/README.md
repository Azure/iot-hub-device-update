# Device Update for IoT Hub - Scripts

## Contents

* [PowerShell Modules](#PowerShell-Modules)
* [Bash Script](#Bash-Script)

## PowerShell Modules

The following PowerShell modules can be used to author and/or import an update to Device Update for IoT Hub (ADU).

| Module | Purpose
| -- | --
| `AduUpdate.psm1` | Create an *import manifest* for importing an update into ADU.
| `AduImportUpdate.psm1` | Create the input (request body) for ADU Import Update REST API and stage the artifacts in Azure Storage Blob container. Requires [Azure Az PowerShell module](https://docs.microsoft.com/en-us/powershell/azure/install-az-ps?view=azps-6.0.0).
| `AduRestApi.psm1` | Call ADU REST API to import an update.

### Usage: Create an import manifest

```powershell
Import-Module .\AduUpdate.psm1

# Create an update that will be compatible with devices from two different manufacturers.
$updateId = New-AduUpdateId `
    -Provider 'Microsoft' `
    -Name 'Toaster' `
    -Version '2.0' `

$compatInfo1 = New-AduUpdateCompatibility `
    -DeviceManufacturer Fabrikam `
    -DeviceModel Toaster

$compatInfo2 = New-AduUpdateCompatibility `
    -DeviceManufacturer Contoso `
    -DeviceModel Toaster

$importManifest = New-AduImportManifest `
    -UpdateId $updateId `
    -UpdateType 'microsoft/swupdate:1' `
    -InstalledCriteria '5' `
    -Compatibility $compatInfo1, $compatInfo2 `
    -Files '.\file1.json', '.\file2.zip'

$importManifest | Out-File '.\importManifest.json' -Encoding UTF8
```

The sample commands above will produce the following import manifest (note that `sizeInbytes` and `sha256` will vary based on the actual files used):

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
  "manifestVersion": "3.0"
}
```

### Usage: Create an update and import to ADU using REST API

To import an update to ADU using REST API

1. [Create an Azure Storage account](https://docs.microsoft.com/en-us/azure/storage/common/storage-account-create?tabs=azure-portal) if you do not already have one. ADU requires update artifacts to be staged in Azure Storage Blob in order to be imported.

2. Create or get a reference to an Azure Storage Blob container. `AduImportUpdate.psm1` module provides a helper cmdlet for this:

    ```powershell
    Import-Module .\AduImportUpdate.psm1

    $AzureSubscriptionId = 'example'
    $AzureResourceGroupName = 'example'
    $AzureStorageAccountName = 'example'
    $AzureBlobContainerName =  'example'

    $container = Get-AduAzBlobContainer `
        -SubscriptionId $AzureSubscriptionId `
        -ResourceGroupName $AzureResourceGroupName `
        -StorageAccountName $AzureStorageAccountName `
        -ContainerName $AzureBlobContainerName
    ```

3. [Create a public Azure Active Directory (AzureAD) Client Application](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-control-access#authenticate-to-device-update-rest-apis-for-publishing-and-management) if you do not already have one. This is required for calling ADU REST API.

4. Obtain an OAuth authorization token for the client application. One option is to use [MSAL.PS](https://github.com/AzureAD/MSAL.PS) PowerShell module:

    ```powershell
    Install-Module MSAL.PS

    $AzureAdClientId = 'example' # AzureAD application (client) ID.
    $AzureAdTenantId = 'example' # AzureAD application directory (tenant) ID.

    $token = Get-MsalToken `
        -ClientId $AzureAdClientId `
        -TenantId $AzureAdTenantId `
        -Scopes 'https://api.adu.microsoft.com/user_impersonation' `
        -Authority https://login.microsoftonline.com/$AzureAdTenantId/v2.0 `
        -Interactive `
        -DeviceCode
    ```

    **Note**: `MSAL.PS` cannot be loaded into the same session as `AzPowershell` due to an [unresolved issue](https://github.com/AzureAD/MSAL.PS/issues/32). For workaround, use them in separate PowerShell sessions.

5. `ImportSampleBundleUpdate.ps1` script contains the sample code for creating a bundle update, and importing it to ADU using
REST API. The update uses this README file as a payload and cannot actually be installed to a device.

    ```powershell
    $AduAccountEndpoint = 'example.api.adu.microsoft.com'
    $AduInstanceId = 'example.instance'

    .\ImportSampleBundleUpdate.ps1 `
        -AccountEndpoint $AduAccountEndpoint `
        -InstanceId $AduInstanceId `
        -Container $container `
        -AuthorizationToken $token `
        -Verbose
    ```

    **Note**: An update version can only be imported once. To run the script multiple times, override the version using `-UpdateVersion` parameter.

### Additional Documentation

To get detailed help information:

```powershell
Get-Help New-AduUpdateId -Detailed
Get-Help New-AduUpdateCompatibility -Detailed
Get-Help New-AduImportManifest -Detailed
Get-Help Get-AduFileHashes -Detailed
Get-Help New-AduImportUpdateInput -Detailed
Get-Help Start-AduImportUpdate -Detailed
Get-Help Wait-AduUpdateOperation -Detailed
```

## Bash Script

Bash script `create-adu-import-manifest.sh` can be used to create an *import manifest* to import an update into Device Update for IoT Hub.

### Usage: Create an import manifest

```bash
# Create an update that will be compatible with devices from two different manufacturers.

./create-adu-import-manifest.sh -p 'Microsoft' -n 'Toaster' -v '2.0' -t 'microsoft/swupdate:1' -i '5' -c Fabrikam,Toaster -c Contoso,Toaster ./file1.json ./file2.zip
```

The sample commands above will produce the following import manifest (note that `sizeInbytes` and `sha256` will vary based on the actual files used):

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
