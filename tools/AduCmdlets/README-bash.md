# create-adu-import-manifest.sh Bash Script

## Overview

Bash script `create-adu-import-manifest.sh` can be used to create an *import manifest* to import an update into Device Update for IoT Hub.

## Usage

To create an import manifest:

```bash
# Create an update that will be compatible with devices from two different manufacturers.

./create-adu-import-manifest.sh -p 'Microsoft' -n 'Toaster' -v '2.0' -t 'microsoft/swupdate:1' -i '5' -c Fabrikam,Toaster -c Contoso,Toaster ./file1.json ./file2.zip
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
