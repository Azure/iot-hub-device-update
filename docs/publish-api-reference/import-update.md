# Import Update

Imports the provided update version to the ADU instance. Files referenced in the POST body must be in an Azure Blob location throughout the duration of the import process.

## HTTP Request

```http
POST https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates?action=import
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |
| action | query | True | enum | Must be `import` |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |
| If-None-Match | False | ETag of previously returned Update entity, for making conditional request. |

**Permissions** <br/>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Request Body

The body of the API is a JSON document referencing the import manifest and the list of file locations. The referenced import manifest is itself also a JSON document that lists the files in the update.

| Name | Required | Type | Description |
| --------- | --------- | --------- | --------- |
| ImportManifest | True | `Import Manifest Download Info` object | Metadata describing the import manifest. The import manifest is a document that describes the files and other metadata about the release. |
| Files | True | Array of `File Download Info` objects | Array of `File Download Info` objects, where each object represents metadata for a single update file. Note that import manifest file must not be specified in this parameter. |

## Responses

| Name | Description |
| --------- | -------------- |
| 202 ACCEPTED | Accepted update import request and background operation URL is specified in `Operation-Location` response header |
| 429 TOO MANY REQUESTS | Too many requests; there is a rate limit on how many operations can be executed within a time period and there is also a limit on how many concurrent import and delete background operations can be executed. |

## Definitions

### Import Manifest Download Info Object

| Name | Required | Type | Description |
| --------- | --------- | --------- | --------- |
| Url | True | URL (string) | Azure Blob location from which the import manifest can be downloaded by ADU. This is typically a read-only SAS-protected blob URL with an expiration set to 4 hours. (See below for import manifest schema) |
| SizeInBytes | True | Int64 | Size in bytes of the import manifest (base 10). |
| Hashes | True | `Hashes` object | A JSON object containing the hash(es) of the file. At least one hash is required. This object can be thought of as a set of key-value pairs where the key is the hash algorithm and the value is the hash of the file calculated using that algorithm. |

### File Download Info Object

| Name | Required | Type | Description |
| --------- | --------- | --------- | --------- |
| Filename | True | string | Update file name as specified inside import manifest. |
| Url | True | URL | Azure Blob location from which the file can be downloaded by ADU. This is typically a read-only SAS-protected blob URL with an expiration set to 4 hours. |

### Hashes Object

| Name | Required | Type | Description |
| --------- | --------- | --------- | --------- |
| Sha256 | True | string | Base64-encoded hash of the file using the SHA-256 algorithm. |

### Import Manifest Schema

| Name | Type | Description | Restrictions |
| --------- | --------- | --------- | --------- |
| UpdateId | `UpdateId` object | Update identity. |
| UpdateType | string | Update type: <ul><li>Specify `microsoft/apt:1` when performing a package-based update using reference agent.</li><li>Specify `microsoft/swupdate:1` when performing an image-based update using reference agent.</li><li>Specify `microsoft/simulator:1` when using sample agent simulator.</li><li>Specify a custom type if developing a custom agent.</li></ul> | <ul><li>Format: `{provider}/{type}:{typeVersion}`</li><li>Maximum of 32 characters total</li></ul> |
| InstalledCriteria | string | String interpreted by the agent to determine if the update completed successfully:  <ul><li>Specify **value** of SWVersion for update type `microsoft/swupdate:1`.</li><li>Specify `{name}-{version}` for update type `microsoft/apt:1`, of which name and version are obtained from the APT file.</li><li>Specify hash of the update file for update type `microsoft/simulator:1`.</li><li>Specify a custom string if developing a custom agent.</li></ul> | Maximum of 64 characters |
| Compatibility | Array of `CompatibilityInfo` objects | Compatibility information of device compatible with this update. | Maximum of 10 items |
| CreatedDateTime | date/time | Date and time at which the update was created. | Delimited ISO 8601 date and time format, in UTC |
| ManifestVersion | string | Import manifest schema version. Specify `2.0`, which will be compatible with `urn:azureiot:AzureDeviceUpdateCore:1` interface and `urn:azureiot:AzureDeviceUpdateCore:4` interface.</li></ul> | Must be `2.0` |
| Files | Array of `File` objects | Update payload files | Maximum of 5 files |

Note: All fields are required.

### UpdateId Object

| Name | Type | Description | Restrictions |
| --------- | --------- | --------- | --------- |
| Provider | string | Provider part of the update identity. | 1-64 characters, alphanumeric, dot and dash. |
| Name | string | Name part of the update identity. | 1-64 characters, alphanumeric, dot and dash. |
| Version | version | Version part of the update identity. | 2 to 4 part, dot separated version number between 0 and 2147483647. Leading zeroes will be dropped. |

### File Object

| Name | Type | Description | Restrictions |
| --------- | --------- | --------- | --------- |
| Filename | string | Name of file | Must be unique within an update |
| SizeInBytes | Int64 | Size of file in bytes. | Maximum of 512 MB per individual file, or 512 MB collectively per update |
| Hashes | `Hashes` object | JSON object containing hash(es) of the file |

## Import Manifest Sample

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