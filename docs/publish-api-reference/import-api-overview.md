# Importing Overview

Importing in ADU is the ability to ingest a device update into ADU, as well as view and delete previously imported updates. For Private Preview, ADU supports rolling out a single update per device, making it ideal for full-image updates that update an entire OS partition at once, or package updates using `apt` and a file that lists all the packages you might want to update on your device.

Each update in ADU is described by an `Update` entity, which contains basic
metadata about the update, like its identity and version, as well as metadata about
the files that make up the update.

The importing workflow is:

1. Build or download a device image update, or create an APT file for package update
2. Create an ADU import manifest describing the update and its files
3. Upload the manifest and files to an Internet-accessible location
4. Call Import Update API, passing it the URLs of the import manifest and the update files
5. Wait for the import process to finish
6. Deploy the update to one or more devices via ADU Management API

## Import APIs

The import APIs enable the ability to upload and manage files that will be deployed to devices. As part of the import flow, an import manifest file will be used to describe an update.  New updates will be scanned for virus or malware as they are copied to distribution servers.

## Getting Started

### Prepare Update File

* For full-image update, have the image file ready.
* For package update, create an [APT file](../agent-reference/apt-manifest.md).

### Create Import Manifest

Create an ADU import manifest, a JSON file that describes an update. The schema and sample can found [here](import-update.md#import-manifest-schema). There is a [PowerShell script](../../tools/AduCmdlets/README.md) that can be used for creating one.

### Use Azure Active Directory authentication for ADU

Before a new update can be submitted to the ADU service, users are required to login using an Azure Active Directory account. [More information on using Azure Active Directory with ADU](../adu-aad-integration.md).

### Submit Import Manifest to Azure Device Update

Upload the import manifest and update file to Azure Blob Storage, and submit the import manifest and update file URLs using [Import Update API](import-update.md).

### Access Imported Update

Retrieve the imported update using [Get Update API](get-update.md).

## Complete List Of Import APIs

* [Import Update](import-update.md)
* [Delete Update](delete-update.md)
* [Get Update](get-update.md)
* [Get File Details](get-file-details.md)
* [Get Operation](get-operation.md)
* [Enumerate Update Providers](enumerate-update-providers.md)
* [Enumerate Update Names](enumerate-update-names.md)
* [Enumerate Update Versions](enumerate-update-versions.md)
* [Enumerate Files](enumerate-files.md)
* [Enumerate Operations](enumerate-operations.md)
